import time, re, os, random
import shlex
import subprocess
import json
from pathlib import Path
from datetime import datetime, timedelta
from typing import List, Dict, Tuple, Optional, Any
from collections import OrderedDict

def clean_compile_options(text: str) -> str:
    """
    从可能包含括号说明的文本中提取纯 gcc 编译选项。
    例如: "-O2 -march=x86-64 (or any architecture...) -fno-schedule-insns"
     -> "-O2 -march=x86-64 -fno-schedule-insns"
    """
    if not text or not isinstance(text, str):
        return ""
    # 移除括号及其内的说明文字
    text = re.sub(r'\([^)]*\)', ' ', text)
    # 移除方括号内的说明
    text = re.sub(r'\[[^\]]*\]', ' ', text)
    # 移除 e.g. 等短语
    text = re.sub(r'\be\.g\.\s*[^,\n]+', ' ', text, flags=re.IGNORECASE)
    # 提取所有 gcc 风格选项: -O, -f, -m, -W 等
    flags = re.findall(r'-(?:O[0-3s]?|[a-zA-Z][a-zA-Z0-9_\-=.,]*)', text)
    return ' '.join(flags) if flags else ""


def extract_compile_commands(prompt: str) -> List[str]:
    """
    Extract compilation option sequences from a prompt.
    Supported formats:
    1. Full gcc/clang commands
    2. Standalone compilation option lists (with optional parenthetical explanations)
    3. Recommended option blocks
    4. [Compile Options] section content
    :param prompt: Input prompt text
    :return: List of cleaned compilation option strings
    """

    compile_options = set()

    # --------------------------------------------------
    # Method 1: Match full gcc/clang command lines
    # --------------------------------------------------
    gcc_pattern = r'(?:^|\n)\s*(?:[^\s]*?(?:gcc|clang))\s+([^\n;]+)'
    gcc_matches = re.findall(gcc_pattern, prompt)
    for match in gcc_matches:
        match = re.sub(r'\S+\.(?:c|cc|cpp|cxx)', '', match)
        match = re.sub(r'-o\s+\S+', '', match)
        options = match.strip()
        if options:
            cleaned = clean_compile_options(options)
            if cleaned:
                compile_options.add(cleaned)
    # --------------------------------------------------
    # Method 2: Extract options under "Recommended" blocks
    # --------------------------------------------------
    rec_pattern = r'Recommended.*?:\s*([\s\S]*?)(?:\n\n|\Z)'
    rec_match = re.search(rec_pattern, prompt, re.IGNORECASE)
    if rec_match:
        block = rec_match.group(1)
        cleaned = clean_compile_options(block)
        if cleaned:
            compile_options.add(cleaned)
    # --------------------------------------------------
    # Method 3: [Compile Options] section (from summarizer output)
    # --------------------------------------------------
    compile_section = re.search(r'\[Compile\s+Options?\][\s\-:]*([\s\S]*?)(?=\n\s*\[|\Z)', prompt, re.IGNORECASE)
    if compile_section:
        content = compile_section.group(1).strip()
        cleaned = clean_compile_options(content)
        if cleaned:
            compile_options.add(cleaned)
    # --------------------------------------------------
    # Method 4: Loose match for sequences starting with -O
    # --------------------------------------------------
    loose_pattern = r'(-O[0-3s](?:\s+-[a-zA-Z][a-zA-Z0-9_\-=.,]*)*)'
    for match in re.findall(loose_pattern, prompt):
        cleaned = clean_compile_options(match)
        if cleaned:
            compile_options.add(cleaned)
    # --------------------------------------------------
    # Method 5: Fallback - any sequence of -flag tokens
    # --------------------------------------------------
    if not compile_options:
        generic_pattern = r'(-(?:O[0-3s]?|[a-zA-Z][a-zA-Z0-9_\-=.,]*)(?:\s+-(?:O[0-3s]?|[a-zA-Z][a-zA-Z0-9_\-=.,]*))+)'
        for match in re.findall(generic_pattern, prompt):
            cleaned = clean_compile_options(match)
            if cleaned:
                compile_options.add(cleaned)
    # --------------------------------------------------
    # Cleanup and normalization
    # --------------------------------------------------
    cleaned = []
    for opt in compile_options:
        opt = re.sub(r'\s+', ' ', opt).strip()
        if opt:
            cleaned.append(opt)
    if len(cleaned) > 1:
        def _rank(x):
            has_o = 1 if re.search(r'-O[0-3s]?\b', x) else 0
            return (-has_o, -len(x))
        cleaned.sort(key=_rank)
    else:
        cleaned.sort()
    return cleaned


def compile_program(
    program_path: str,
    compile_options: Optional[str] = None,
    gcc_path: Optional[str] = None,
    timeout_sec: int = 60,
) -> Tuple[bool, Optional[str]]:
    """
    Compile a single source program and generate an executable binary.

    Functionality:
    - Compiles the source file specified by program_path
    - Produces an executable with the same base name and ".out" extension
    - Supports custom compilation options (e.g., "-O2 -fsanitize=address -fopenmp")
    - Captures and returns compilation errors (stderr)

    Parameters:
    :param program_path: Path to the source file (e.g., ".../test.c")
    :param compile_options: Compilation option string (excluding compiler path and input/output files).
                            If None, "-O3" is used by default.
    :param gcc_path: Path to the compiler executable. If None, a default path is used.
    :param timeout_sec: Compilation timeout in seconds (prevents hanging processes)

    Returns:
    :return: (success_flag, error_message)
             - On success: (True, None)
             - On failure: (False, "Compilation error: ...")
    """

    src = Path(program_path)
    if not src.exists() or not src.is_file():
        return False, f"Compilation error: Source file does not exist: {program_path}"

    if gcc_path is None:
        gcc_path = "gcc-build/bin/gcc"

    out = src.with_suffix(".out")
    options_str = compile_options.strip() if compile_options else "-O3"
    cmd = [gcc_path] + shlex.split(options_str) + [str(src), "-o", str(out)]
    print(f"[Compile] {' '.join(cmd)}")
    try:
        subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            timeout=timeout_sec,
        )
        return True, None
    except subprocess.TimeoutExpired:
        return False, f"Compilation error: Timeout (> {timeout_sec}s)"
    except subprocess.CalledProcessError as e:
        stderr = (e.stderr or "").strip()
        stdout = (e.stdout or "").strip()
        message = stderr if stderr else stdout
        if not message:
            message = "Unknown compilation error (no output captured)"

        return False, f"Compilation error: {message}"


def check_lines_coverage(block_text: str, gcov_file: str) -> Tuple[bool, List[int]]:
    """
    Check whether the target lines (extracted from an input uncovered block text)
    become covered in a given .gcov file.

    Parameters:
    :param block_text: The input block text (original snippet with "#####" markers)
    :param gcov_file: Path to the .gcov file to check

    Returns:
    :return: (covered_any, target_lines)
             - covered_any: True if at least one target line is now covered (numeric count)
             - target_lines: list of extracted target line numbers
    """
    # Extract line numbers that are marked as "#####"
    # Example line: "#####:  1169:   case '=':"
    target_line_pattern = re.compile(r'^\s*#####:?\s*(\d+)\s*:', re.MULTILINE)
    target_lines = [int(x) for x in target_line_pattern.findall(block_text)]
    target_lines = sorted(set(target_lines))

    if not target_lines:
        return False, []
    # Build a lookup set for faster membership tests
    target_set = set(target_lines)
    # Parse gcov file lines and detect if any target line is covered (numeric count)
    # gcov line format example:
    #   "    12:  1169:   case '=':"
    #   "##### :  1169:   case '=':"   (some gcov versions may include a space before ':')
    #   "#####:  1169:   case '=':"
    #   "    -:  1171:"
    gcov_line_pattern = re.compile(r'^\s*(?P<count>[#\-\d]+)\s*:\s*(?P<lineno>\d+)\s*:', re.MULTILINE)

    try:
        with open(gcov_file, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                m = gcov_line_pattern.match(line)
                if not m:
                    continue
                lineno = int(m.group("lineno"))
                if lineno not in target_set:
                    continue

                count = m.group("count")
                # If the original marker "#####" becomes a digit count, treat as covered
                if count.isdigit():
                    return True, target_lines

        return False, target_lines

    except FileNotFoundError:
        raise FileNotFoundError(f"gcov file not found: {gcov_file}")


def parse_gcov_file(gcov_path: str) -> Dict[int, Tuple[str, str]]:
    """Parse gcov file, return {line_num: (count, code)}."""
    result: Dict[int, Tuple[str, str]] = {}
    pattern = re.compile(r'^\s*(?P<count>[#\-\d]+)\s*:\s*(?P<lineno>\d+)\s*:(?P<code>.*)$')
    try:
        with open(gcov_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                m = pattern.match(line)
                if m:
                    ln = int(m.group("lineno"))
                    result[ln] = (m.group("count").strip(), m.group("code"))
    except FileNotFoundError:
        pass
    return result


def collect_all_gcov_state(coverage_dir: str) -> Dict[str, Dict[int, Tuple[str, str]]]:
    """Parse all .gcov files in coverage_dir, return {gcov_filename: {line_num: (count, code)}}."""
    result: Dict[str, Dict[int, Tuple[str, str]]] = {}
    for f in Path(coverage_dir).glob("*.gcov"):
        result[f.name] = parse_gcov_file(str(f))
    return result


def compute_coverage_improvements(
    target_file: str,
    cov_before: Dict[str, Dict[int, Tuple[str, str]]],
    cov_after: Dict[str, Dict[int, Tuple[str, str]]],
) -> Tuple[List[str], str]:
    """
    Compare before/after coverage.
    :return: (improved_other_files, improved_in_file)
    - improved_other_files: list of file names (excluding target) that had newly covered lines
    - improved_in_file: target file's newly covered lines, format "line_num: code"
    """
    target_basename = os.path.basename(target_file.replace("\\", "/"))
    improved_other_files: List[str] = []
    improved_in_file_parts: List[str] = []

    for gcov_name, after_data in cov_after.items():
        before_data = cov_before.get(gcov_name, {})
        file_basename = gcov_name.replace(".gcov", "") if gcov_name.endswith(".gcov") else gcov_name

        newly_covered = []
        for ln, (new_count, new_code) in after_data.items():
            old = before_data.get(ln, ("#####", ""))
            old_count = old[0]
            code = old[1] or new_code
            if old_count == "#####" and new_count.isdigit():
                newly_covered.append((ln, code))

        if not newly_covered:
            continue

        if file_basename == target_basename or gcov_name == target_basename + ".gcov":
            for ln, code in sorted(newly_covered):
                improved_in_file_parts.append(f"{ln}:{code}")
        else:
            improved_other_files.append(file_basename)

    improved_in_file = "\n".join(improved_in_file_parts) if improved_in_file_parts else ""
    return improved_other_files, improved_in_file


def parse_requirements(summary_text: str) -> Dict[str, str]:
    """
    Parse summarizer output to extract [Coverage Goal], [Compile Options], [Basic Block N].
    Returns dict with keys: coverage_goal, compile_options, target_block
    """
    result = {"coverage_goal": "", "compile_options": "-O2", "target_block": ""}
    if not summary_text or not isinstance(summary_text, str):
        return result

    # Match [Section Name] - content until next [ or end
    section_pattern = re.compile(
        r'\[([^\]]+)\]\s*[-:]?\s*(.*?)(?=\n\s*\[|\Z)',
        re.DOTALL
    )
    blocks = []
    for m in section_pattern.finditer(summary_text):
        name = m.group(1).strip()
        content = m.group(2).strip()
        name_lower = name.lower()
        if "coverage goal" in name_lower:
            result["coverage_goal"] = content
        elif "compile" in name_lower and "option" in name_lower:
            opts = clean_compile_options(content) if content else ""
            result["compile_options"] = opts if opts else "-O2"
        elif "basic block" in name_lower:
            blocks.append(content)

    if blocks:
        result["target_block"] = "\n\n".join(blocks)
    return result


def format_uncovered_block(block: Dict) -> str:
    """Format uncovered block for check_lines_coverage (#####: line: code format)."""
    ub = block.get("uncovered_block", [])
    return "\n".join(f"{e['count']:>5}: {e['line_num']:5d}:{e['code']}" for e in ub)


def format_block_for_prompt(block: Dict) -> Tuple[str, str]:
    """Format block into uncovered_code and covered_code strings for summarization prompt."""
    ub = block.get("uncovered_block", [])
    uncovered = "\n".join(f"{e['count']:>5}: {e['line_num']:5d}:{e['code']}" for e in ub)
    ctx = block.get("covered_context")
    if ctx:
        covered = "\n".join(f"{e['count']:>5}: {e['line_num']:5d}:{e['code']}" for e in ctx)
    else:
        covered = "(no covered context)"
    return uncovered, covered


def make_summarization_generation_prompt(
    file_path: str,
    uncovered_code: str,
    covered_code: str) -> str:
    """
    Construct a coverage-guided summarization prompt  generation following the structured template.
    Parameters:
    :param file_path: Target compiler source file path
    :param uncovered_code: Uncovered code snippet with line numbers
    :param covered_code: Covered code snippet with line numbers
    Returns:
    :return: A structured prompt string for compiler coverage analysis
    """
    return f"""[Role]
    You are an expert Compiler Developer and Fuzzing Specialist. Your goal is to analyze compiler source code to assist in generating test cases that achieve high code coverage. You possess deep knowledge of compiler architecture (e.g., LLVM, GCC) and control flow analysis.
    [Input Data]
    1. Target File Path: The location of the source file:
    {file_path}
    2. Uncovered Code and Line Number:
    {uncovered_code}
    3. Covered Code and Line Number:
    {covered_code}
    [Instructions]
    Please perform the analysis in the following steps:
    Step 1: Analyze the Target File Path and the code content and infer the functional role of this file.
    Step 2: Compare the Covered and Uncovered code segments.
    2.1 Summarize the functionality of the code that is already covered.
    2.2 Analyze the Uncovered code and divide them into Basic Blocks. Why were they missed?
    2.3 Evaluate the "Coverage Diversity": What makes the uncovered path functionally different from the covered path? 
        (e.g., handles edge cases, specific data types, rare optimization triggers, etc.)
    Step 3: Derive the Coverage Goal based on the uncovered code and the "Coverage Diversity".
        Abstract a concise coverage goal that describes what compiler behavior or path should be triggered.
    Step 4: Derive the required characteristics of the test input (C/C++ code structure) to reach the coverage goal.
        (e.g., specific control flow constructs, loops, vector types, deep nesting, volatile variables, etc.)
    Step 5: Derive the required compilation options to reach this coverage goal.
        (e.g., -O2, -O3, -march=..., target backend, specific warnings or optimization flags)
    [Output Goal]
    Output in the following structure. Use plain text only, do NOT wrap in markdown code blocks (```):
    [Coverage Goal] - Overall coverage objective
    [Compile Options] - Required compilation options
    [Basic Block 1] - Required program structure to cover it
    [Basic Block 2] - Required program structure to cover it
    ...
    Output the complete structured response. All sections are required.
    """

def make_program_generate_prompt(
    failures_case: str,
    target_file: str,
    coverage_goal: str,
    compile_options: str,
    target_block: str) -> str:
    """
    Construct a synthesized compiler test generation prompt.
    Inputs:
    - failures_case: the entire failure-case block 
    - target_file: target source file to focus on (e.g., gcc/c-family/c-ada-spec.cc)
    - coverage_goal: concise goal describing what to cover
    - compile_options: compilation options required to reach the goal (e.g., -O2 -fdump-ada-spec)
    - target_block: uncovered/basic-block snippet or structure requirements to cover
    Returns:
    - A structured generation prompt string for program synthesis.
    """
    return f"""[Role]
    You are an expert C/C++ fuzzer and compiler test generator with deep understanding of compiler mechanisms.
    Please generate test programs that satisfy the given requirements, target the specified coverage targets,
    and explore compiler paths beyond those explicitly described.
    [Failures Case]
    {failures_case}
    [Target Requirements]
    Target compiler file:
    {target_file}
    Coverage goal:
    {coverage_goal}
    The compilation options targeted by the generated test program structure:
    {compile_options}
    The basic blocks in {target_file} to be covered by the generated program structure:
    {target_block}
    [General Requirements]
    Standards & Compliance: : ISO C99 compliant; self-contained and compilable with a standard gcc/llvm compiler; no undefined behavior or compiler extensions.
    Program Structure: Include a main function and multiple helper functions; use non trivial control flow (e.g., nested conditionals, loops, and function calls).
    Scale & Complexity: Approximately 80–120 lines of code; structurally complex enough to allow diverse compilation paths while remaining readable and well-formed.
    Determinism & Inputs: No external inputs, system calls, or I/O dependencies; program behavior should be deterministic and reproducible.
    [Instruction]
    1. Generate test programs that match the given targeted requirements.
    2.Avoid generating test programs that are structurally similar to given failure cases, based on reflection results.
    3. Ensure the programs meet the general requirements and have a well-structured design.
    4. Output complete, compilable C/C++ test program.
    [Output]
    Output a complete C/C++ test program only.
    Do NOT include explanations or markdown fences.
    """


def main():
    """
    Coverage-driven program generation pipeline.
    """
    # ========== Config (similar to SOLAR main.py) ==========
    INITIAL_PROMPT = (
        "Generate ONE ISO C99 program that exercises GCC compiler optimizations. "
        "Must include main, use non-trivial control flow (loops, conditionals, function calls). "
        "Recommended Compilation Options: -O2 -fno-strict-aliasing\n"
        "Output only code."
    )
    GCC_PATH = "../gcc-ztc-build/bin/gcc"
    GCOV_PATH = "gcov-13"  
    COVERAGE_DIR = "xxx/GapSmith/coverage"
    OUTPUT_DIR = "xxx/GapSmith/programs"
    PROMPT_DIR = "xxx/GapSmith/prompts"
    BAD_CASES_DIR = "xxx/GapSmith/bad_cases"

    source_dirs = [
        "xxx/gcc-ztc-build/gcc",
    ]
    target_dirs = [
        "xxx/gcc-14.3.0/gcc",
    ]

    # Time limit: run for 2 hours (or set end_times list for multiple checkpoints)
    run_duration_hours = 2.0
    end_time = datetime.now() + timedelta(hours=run_duration_hours)
    LOOP_BATCH_SIZE = 50  # 每轮生成的程序数量

    # Ensure directories exist
    for d in [COVERAGE_DIR, OUTPUT_DIR, PROMPT_DIR, BAD_CASES_DIR]:
        Path(d).mkdir(parents=True, exist_ok=True)

    # Import algorithm modules
    import sys
    sys.path.insert(0, str(Path(__file__).parent))
    from algorithm.generate import BatchCodeGenerator
    from algorithm.collect import GcovRunner
    from algorithm.sort import GapSmithSelector
    from algorithm.uncovered_analyzer import UncoveredBlockAnalyzer
    from algorithm.summarize import UncoveredRequirementSummarizer
    from algorithm.find_bad import BadCaseFinder

    api_key = os.getenv("DEEPSEEK_API_KEY")
    if not api_key:
        raise RuntimeError("Set DEEPSEEK_API_KEY in your environment")

    generator = BatchCodeGenerator(
        api_key=api_key,
        output_dir=OUTPUT_DIR,
        model="deepseek-chat",
        max_workers=1,
    )
    summarizer = UncoveredRequirementSummarizer(
        api_key=api_key,
        output_dir="summaries",  # relative path, created in cwd
    )
    finder = BadCaseFinder(bad_cases_dir=BAD_CASES_DIR)

    # ---------- Phase 1: Initial program generation ----------
    print("[Phase 1] Initial program generation...")
    gen_stats = generator.generate_batch(batch_size=1, base_prompt=INITIAL_PROMPT, prefix="init")
    batch_dir = gen_stats.get("batch_dir")
    if not batch_dir:
        print("[Error] No batch directory from generator")
        return

    # Find generated .c file
    batch_path = Path(batch_dir)
    c_files = list(batch_path.glob("*.c"))
    if not c_files:
        print("[Error] No .c file generated")
        return
    program_path = str(c_files[0])

    # Extract compile options and compile
    opts_list = extract_compile_commands(INITIAL_PROMPT)
    compile_opts = opts_list[0] if opts_list else "-O2"
    success, err_msg = compile_program(program_path, compile_opts, GCC_PATH)
    if not success:
        print(f"[Compile] Initial program failed: {err_msg}")
    else:
        print("[Compile] Initial program compiled successfully")

    # Collect coverage
    orig_cwd = os.getcwd()
    try:
        os.chdir(COVERAGE_DIR)
        runner = GcovRunner(source_dirs, target_dirs, output_dir=COVERAGE_DIR, gcov_path=GCOV_PATH)
        runner.run()
        print(f"[Coverage] Collected, avg: {runner.compute_average_coverage():.2f}%")
    finally:
        os.chdir(orig_cwd)

    # ---------- Phase 2: Coverage-driven loop ----------
    # 记录每个文件连续覆盖失败的次数，超过10次不再选择，成功一次则清零
    file_failure_count: Dict[str, int] = {}
    FAILURE_THRESHOLD = 10

    iteration = 0
    while datetime.now() < end_time:
        iteration += 1
        print(f"\n[Iteration {iteration}]")

        if not os.path.isdir(COVERAGE_DIR):
            print("[Error] Coverage dir not found")
            break

        # 2.1 Select target file
        selector = GapSmithSelector(report_folder=COVERAGE_DIR)
        selector.parse_all_reports()
        if not selector.targets:
            print("[Warning] No targets in coverage report, re-collecting...")
            try:
                os.chdir(COVERAGE_DIR)
                runner = GcovRunner(source_dirs, target_dirs, output_dir=COVERAGE_DIR, gcov_path=GCOV_PATH)
                runner.run()
            finally:
                os.chdir(orig_cwd)
            selector.parse_all_reports()
        if not selector.targets:
            print("[Error] Still no targets, skipping iteration")
            continue

        selector.calculate_metrics()

        exclude_files = {f for f, c in file_failure_count.items() if c >= FAILURE_THRESHOLD}
        target_info = selector.select_next_target(exclude_files=exclude_files)
        if not target_info:
            if exclude_files:
                print(f"  [Warning] All candidates excluded (failure >= {FAILURE_THRESHOLD})")
            continue
        target_file = target_info.get("filename", "")
        if not target_file:
            continue
        print(f"  Target file: {target_file} (consecutive failures: {file_failure_count.get(target_file, 0)})")

        # 2.2 Select target block (uncovered)
        gcov_dir = COVERAGE_DIR
        base_name = os.path.basename(target_file.replace("\\", "/"))
        gcov_file = os.path.join(gcov_dir, base_name + ".gcov")
        if not os.path.isfile(gcov_file):
            print(f"  [Warning] Gcov file not found: {gcov_file}")
            continue

        analyzer = UncoveredBlockAnalyzer(gcov_file, context_limit=20)
        analyzer.parse()
        if not analyzer.blocks:
            print("  [Warning] No uncovered blocks in file")
            continue
        analyzer.blocks.sort(key=lambda b: b["block_size"], reverse=True)
        target_block = analyzer.blocks[0]
        uncovered_block_text = format_uncovered_block(target_block)

        # 2.3 Build summarization prompt and call summarize
        uncovered_code, covered_code = format_block_for_prompt(target_block)
        sum_prompt = make_summarization_generation_prompt(target_file, uncovered_code, covered_code)
        print(f"  [Summarization Prompt] {sum_prompt}")
        requirements = summarizer.run(sum_prompt, iteration_index=iteration, retry_times=3)
        if not requirements:
            print("  [Warning] Summarization failed")
            continue

        parsed = parse_requirements(requirements)
        coverage_goal = parsed.get("coverage_goal") or "Cover the uncovered compiler code blocks"
        compile_options = clean_compile_options(parsed.get("compile_options") or "-O2") or "-O2"
        target_block_str = parsed.get("target_block") or uncovered_code

        # 2.4 Find bad cases
        failures_case = ""
        try:
            failures_case = finder.run(target_file)
        except FileNotFoundError:
            pass

        # 2.5 Build full program generation prompt
        full_prompt = make_program_generate_prompt(
            failures_case=failures_case,
            target_file=target_file,
            coverage_goal=coverage_goal,
            compile_options=compile_options,
            target_block=target_block_str,
        )
   
        # 2.6 Generate programs
        gen_stats = generator.generate_batch(batch_size=LOOP_BATCH_SIZE, base_prompt=full_prompt, prefix="iter")
        batch_dir = gen_stats.get("batch_dir")
        if not batch_dir:
            continue
        batch_path = Path(batch_dir)
        c_files = sorted(batch_path.glob("*.c"))
        if not c_files:
            continue

        # 2.7 编译前收集覆盖率（解析当前 .gcov 作为 before 状态）
        cov_before = collect_all_gcov_state(COVERAGE_DIR)

        # 2.8 Compile all programs and concatenate compile errors
        compile_errors: List[str] = []
        for c_file in c_files:
            success, compile_err = compile_program(str(c_file), compile_options, GCC_PATH)
            if not success:
                compile_errors.append(f"[{c_file.name}] {compile_err or 'Unknown error'}")
        compile_status = "\n".join(compile_errors) if compile_errors else "All programs compiled successfully"

        # 2.9 编译后收集覆盖率
        try:
            os.chdir(COVERAGE_DIR)
            runner = GcovRunner(source_dirs, target_dirs, output_dir=COVERAGE_DIR, gcov_path=GCOV_PATH)
            runner.run()
        finally:
            os.chdir(orig_cwd)

        cov_after = collect_all_gcov_state(COVERAGE_DIR)
        improved_other_files_list, improved_in_file_str = compute_coverage_improvements(
            target_file, cov_before, cov_after
        )
        improved_other_files_str = ", ".join(improved_other_files_list) if improved_other_files_list else ""

        # 2.10 Check if target block is covered
        covered_any, _ = check_lines_coverage(uncovered_block_text, gcov_file)

        # 每轮都保存 prompt（用时间戳命名）
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        prompt_path = Path(PROMPT_DIR) / f"prompt_{ts}.txt"
        prompt_path.write_text(full_prompt, encoding="utf-8")
        print(f"  [Prompt] Saved: {prompt_path}")

        if covered_any:
            # Covered: 清零失败计数
            file_failure_count[target_file] = 0
            print(f"  [Covered] Block covered!")
        else:
            # Not covered: 失败计数 +1
            file_failure_count[target_file] = file_failure_count.get(target_file, 0) + 1
            # Not covered: save bad case
            folder_name = target_file.replace("\\", "/").replace("/", "__")
            bad_folder = Path(BAD_CASES_DIR) / folder_name
            bad_folder.mkdir(parents=True, exist_ok=True)
            existing = list(bad_folder.glob("case*.json"))
            case_num = len(existing) + 1
            case_path = bad_folder / f"case{case_num}.json"
            case_data = {
                "prompt_base": full_prompt,
                "compilation_status": compile_status,
                "improved_other_files": improved_other_files_str,
                "improved_in_file": improved_in_file_str[:500] if improved_in_file_str else "",
            }
            case_path.write_text(json.dumps(case_data, indent=2, ensure_ascii=False), encoding="utf-8")
            print(f"  [Not covered] Bad case saved: {case_path}")

    print("\n[Done] Coverage-driven loop finished.")


if __name__ == "__main__":
    main()

