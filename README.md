# GapSmith
GapSmith is a coverage-driven compiler testing framework that leverages large language models (LLMs) and execution feedback to iteratively generate high-quality test programs for improving compiler coverage, especially targeting hard-to-cover regions.

## algorithm
The algorithm/ directory contains the core modules of GapSmith, implementing a full pipeline from coverage collection to test generation:
- `algorithm/collect.py` - Collects line-level coverage data (e.g., via gcov) from specified compiler source directories.
- `algorithm/uncovered_analyzer.py` - Identifies uncovered code regions and extracts their structural context (e.g., nearby covered lines) to guide targeted generation.
- `algorithm/sort.py` - Ranks candidate files using a coverage-driven scoring strategy to prioritize high-impact targets.
- `algorithm/find_bad.py` -  Retrieves representative failure cases (i.e., ineffective or misaligned test programs) for a given target file, enabling reflective prompt refinement.
- `algorithm/summarize.py` - Summarizes uncovered regions into structured requirements, including functional roles, triggering conditions, and relevant compilation options.
- `algorithm/generate.py` - Synthesizes test programs using LLMs based on summarized requirements and historical feedback.

## results
The results/ directory contains evaluation outputs across multiple experimental settings:
- `cold_start_results.json` - Performance under cold-start conditions without prior coverage.
- `coverage_improvement_results.json` - Coverage over original test cases.
- `whitefox_comparison_results.json` - Overall comparison with WhiteFox.
- `whitefox_case_results.json` - Fine-grained, case-by-case comparison with WhiteFox.
- `ablation_study_results.json` - Impact analysis of individual components in GapSmith.
- `cost_analysis_results.json` - Token consumption and computational cost of LLM-based components.
- `base_model_comparison_results.json` - Performance comparison across different base models.
- `failure_results.json` - Statistics and categorization of failure cases during generation.

## `main.py` - The main entry point of GapSmith, orchestrating the full pipeline.
**Usage**:
```bash
DEEPSEEK_API_KEY="xx" python main.py
```
note: Please ensure all paths (e.g., compiler source directories and output folders) are correctly configured before execution.

**Coverage Collection**:
For orignial test suite, you can collect as:
GCC:
```bash
runtest --tool gcc --tool_exec ${INSTALL_DIR}/bin/gcc - srcdir ${SRCDIR}/testsuite
runtest --tool g++ --tool_exec ${INSTALL_DIR}/bin/g++ - srcdir ${SRCDIR}/testsuite
```
LLVM:
```bash
ninja check-all
```
For generated test suite, you can direct compile them under different complation options.

## Data storage folders
- **programs** - Stores generated test programs (All generated test programs and intermediate artifacts are available at: https://drive.google.com/drive/folders/11CEIDCCGAiiyR9BFBnZkQ7seJCSIwdp0?usp=sharing).
- `prompts` - Stores constructed prompts used for LLM-based generation.
- `summaries` - Stores structured summaries of uncovered regions.
- `bad_cases` - Stores representative failure cases for reflective learning.
