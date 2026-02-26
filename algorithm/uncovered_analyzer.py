import os
import re


class UncoveredBlockAnalyzer:
    def __init__(self, gcov_file: str, context_limit: int = 20):
        """
        Uncovered block analyzer
        Function:
        - Parse .gcov file
        - Extract consecutive uncovered blocks (#####)
        - Extract at most context_limit lines of "covered code" before and after each uncovered block as context
        :param gcov_file: .gcov file path
        :param context_limit: context limit
        """
        self.gcov_file = gcov_file
        self.context_limit = context_limit
        self.line_pattern = re.compile(
            r'^\s*(?P<count>[#\-\d]+):\s*(?P<line_num>\d+):(?P<code>.*)$'
        )
        self.entries = []  # [{"count","line_num","code"}...]
        self.blocks = []

    def _is_pure_comment(self, code: str) -> bool:
        """
        Check if the line is a pure comment line (avoid treating comments as executable code)
        """
        s = code.strip()
        return s.startswith('//') or s.startswith('/*') or s.startswith('*')

    def _is_code_line(self, code: str) -> bool:
        """
        Check if the line is a valid code line (not empty, not pure comment)
        """
        return bool(code.strip()) and (not self._is_pure_comment(code))

    def _is_covered(self, count: str, code: str) -> bool:
        """
        Check if the line is a covered code line
        Conditions:
        - count is a number (execution count)
        - is a valid code line
        """
        return count.isdigit() and self._is_code_line(code)

    def _is_uncovered_exec(self, count: str, code: str) -> bool:
        """
        Check if the line is a truly uncovered executable code
        Conditions:
        - count == '#####'
        - is a valid code line
        """
        return (count == '#####') and self._is_code_line(code)

    def _is_neutral(self, count: str) -> bool:
        """
        Check if the line is a neutral line
        Conditions:
        - count == '-'
        """
        return count == '-'

    def parse(self):
        """
        Parse gcov file, and build uncovered blocks
        """

        if not os.path.isfile(self.gcov_file):
            raise FileNotFoundError(f"file not found: {self.gcov_file}")
        self.entries.clear()
        self.blocks.clear()
        with open(self.gcov_file, 'r', encoding='utf-8', errors='ignore') as f:
            for raw in f:
                m = self.line_pattern.match(raw)
                if not m:
                    continue

                self.entries.append({
                    "count": m.group("count").strip(),
                    "line_num": int(m.group("line_num")),
                    "code": m.group("code")
                })

        in_block = False
        start_idx = None
        cur_block = []
        uncovered_exec_count = 0  
        for idx, e in enumerate(self.entries):
            c = e["count"]
            code = e["code"]
            if self._is_uncovered_exec(c, code):
                if not in_block:
                    in_block = True
                    start_idx = idx
                    cur_block = []
                    uncovered_exec_count = 0

                cur_block.append(e)
                uncovered_exec_count += 1
                continue
            if in_block and self._is_neutral(c):
                cur_block.append(e)
                continue
            if in_block:
                end_idx = idx - 1
                self._save_block(start_idx, end_idx, cur_block, uncovered_exec_count)
                in_block = False
                start_idx = None
                cur_block = []
                uncovered_exec_count = 0
        if in_block and start_idx is not None and cur_block:
            self._save_block(start_idx, len(self.entries) - 1, cur_block, uncovered_exec_count)

    def _collect_context(self, start_idx: int, end_idx: int):
        """
        Collect at most context_limit lines of "covered code" before and after each uncovered block
        """
        before = []
        i = start_idx - 1
        while i >= 0 and len(before) < self.context_limit:
            e = self.entries[i]
            if self._is_covered(e["count"], e["code"]):
                before.append(e)
            i -= 1
        before.reverse()
        after = []
        i = end_idx + 1
        while i < len(self.entries) and len(after) < self.context_limit:
            e = self.entries[i]
            if self._is_covered(e["count"], e["code"]):
                after.append(e)
            i += 1

        return before + after

    def _save_block(self, start_idx: int, end_idx: int, uncovered_block, uncovered_exec_count: int):
        """
        Save block:
        - block_size only counts ##### executable lines
        - if there is no context, covered_context = None
        """
        covered_context = self._collect_context(start_idx, end_idx)

        if not covered_context:
            covered_context = None

        self.blocks.append({
            "covered_context": covered_context,
            "uncovered_block": uncovered_block,
            "block_size": uncovered_exec_count
        })

    def print_blocks(self, max_blocks: int = 1):
        """
        Print uncovered blocks
        """
        self.blocks.sort(key=lambda b: b["block_size"], reverse=True)
        blocks_to_print = self.blocks[:max_blocks]
        print(f"\nfile: {os.path.basename(self.gcov_file)}")
        print(f"uncovered blocks: {len(self.blocks)} (showing {len(blocks_to_print)})")
        print("=" * 90)
        for i, b in enumerate(blocks_to_print, 1):
            ub = b["uncovered_block"]
            start_line = ub[0]["line_num"]
            end_line = ub[-1]["line_num"]
            print(f"\n[Block {i}] uncovered lines {start_line}-{end_line} "
                  f"(exec_uncovered_size={b['block_size']})")
            print("\ncovered_context:")
            if b["covered_context"] is None:
                print("(empty)")
            else:
                for e in b["covered_context"]:
                    print(f"{e['count']:>5}: {e['line_num']:5d}:{e['code']}")
            print("\nuncovered_block:")
            for e in ub:
                print(f"{e['count']:>5}: {e['line_num']:5d}:{e['code']}")

            print("-" * 90)
