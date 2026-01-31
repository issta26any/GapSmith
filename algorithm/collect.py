import os
import subprocess
import sys
import re
from typing import List

class GcovRunner:
    def __init__(self, source_dirs: List[str], target_dirs: List[str], output_dir: str = "."):
        if len(source_dirs) != len(target_dirs):
            raise ValueError("source_dirs and target_dirs must have the same length")
        self.source_dirs = source_dirs
        self.target_dirs = target_dirs
        self.output_dir = output_dir
        self.file_pattern = re.compile(r"File '(.*\.cc)'")
        self.coverage_pattern = re.compile(r"Lines executed:([\d.]+)% of (\d+)")
        self.coverage_results = [] 
        self.processed_files = set()

    def run(self):
        self.coverage_results.clear()
        self.processed_files.clear()  

        for src_dir, tgt_dir in zip(self.source_dirs, self.target_dirs):
            if not os.path.isdir(src_dir) or not os.path.isdir(tgt_dir):
                print(f"path not found: {src_dir} or {tgt_dir}")
                continue

            txt_name = os.path.basename(os.path.normpath(tgt_dir)) + ".txt"
            output_path = os.path.join(self.output_dir, txt_name)

            with open(output_path, 'w') as out:
                for root, _, files in os.walk(tgt_dir):
                    for file in files:
                        if file.endswith('.cc'):
                            cc_path = os.path.join(root, file)
           
                            abs_path = os.path.abspath(cc_path)
                            if abs_path in self.processed_files:
                                continue
                            
                            self.processed_files.add(abs_path)

                            try:
                                proc = subprocess.run(
                                    ['gcov-13', '-o', src_dir, cc_path],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT,
                                    encoding='utf-8',
                                    cwd=os.getcwd()
                                )
                                current_file = None
                                
                                for line in proc.stdout.splitlines():
                                    file_match = self.file_pattern.search(line)
                                    if file_match:
                                        current_file = file_match.group(1)

                                    cov_match = self.coverage_pattern.search(line)
                                    
                                    if cov_match and current_file and current_file.endswith(file): 
                                        coverage = float(cov_match.group(1))
                                        total_lines = int(cov_match.group(2))
                                        out.write(f"{current_file}: {coverage:.2f}% of {total_lines} lines\n")
                                        self.coverage_results.append((
                                            os.path.basename(current_file),
                                            coverage,
                                            total_lines
                                        ))
                                        break
                            except Exception as e:
                                print(f" {cc_path} : {e}")

    def compute_average_coverage(self):
        
        if not self.coverage_results:
            return 0.0
        total_exec = sum((cov / 100) * lines for _, cov, lines in self.coverage_results)
        total_lines = sum(lines for _, _, lines in self.coverage_results)
        return (total_exec / total_lines) * 100 if total_lines > 0 else 0.0
if __name__ == "__main__":
    source_dirs = [
        "../gcc-ztc-build/gcc/c",
        "../gcc-ztc-build/gcc/c-family",
        "../gcc-ztc-build/gcc/common",
        "../gcc-ztc-build/gcc/cp",
        "../gcc-ztc-build/gcc/lto",
        "../gcc-ztc-build/gcc"
    ]

    target_dirs = [
        "../gcc-14.3.0/gcc/c",
        "../gcc-14.3.0/gcc/c-family",
        "../gcc-14.3.0/gcc/common",
        "../gcc-14.3.0/gcc/cp",
        "../gcc-14.3.0/gcc/lto",
        "../gcc-14.3.0/gcc"
    ]
    
    runner = GcovRunner(source_dirs, target_dirs)
    runner.run()
    avg = runner.compute_average_coverage()
    print(f"Have done")