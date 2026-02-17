import os
import re
import math
import random
from typing import List, Dict

class GapSmithSelector:
    """
    GapSmith target file selector
    """
    def __init__(self, report_folder: str):
        """
        :param report_folder: coverage report txt files directory
        """
        self.report_folder = report_folder
        self.pattern = re.compile(r"^\s*(?P<file>.*?):\s+(?P<percent>[\d.]+)%\s+of\s+(?P<lines>\d+)\s+lines")
        self.targets: List[Dict] = []
        self.t_param = 10 

    def parse_all_reports(self):
        """
        Parse all coverage report txt files
        """
        if not os.path.isdir(self.report_folder):
            print(f"Directory not found: {self.report_folder}")
            return
        for fname in os.listdir(self.report_folder):
            if fname.endswith(".txt"):
                file_path = os.path.join(self.report_folder, fname)
                with open(file_path, "r", encoding="utf-8") as f:
                    for line in f:
                        match = self.pattern.search(line)
                        if match:
                            filename = match.group("file").strip()
                            percent = float(match.group("percent"))
                            lines_count = int(match.group("lines"))
                            coverage_ratio = percent / 100.0
                            
                            self.targets.append({
                                'filename': filename,
                                'Lf': lines_count,
                                'Cf': coverage_ratio,
                                'Sf': 0.0,
                                'Wf': 0.0,
                                'Pf': 0.0
                            })

    def calculate_metrics(self):
        """
        1. Sf = Lf * (1 - Cf)^2 
        2. Wf = Sf / Sum(Sf)
        3. Pf = 1 - (1 - Wf)^t
        """
        if not self.targets:
            return
        total_score_S = 0.0
        for target in self.targets:
            lf = target['Lf']
            cf = target['Cf']
            sf = lf * math.pow((1.0 - cf), 2)
            target['Sf'] = sf
            total_score_S += sf
        if total_score_S == 0:
            print("Total score is zero.")
            return
        for target in self.targets:
            wf = target['Sf'] / total_score_S
            pf = 1.0 - math.pow((1.0 - wf), self.t_param)
            
            target['Wf'] = wf
            target['Pf'] = pf
        self.targets.sort(key=lambda x: x['Pf'], reverse=True)

    def select_next_target(self):
        """
        Select next target file
        """
        if not self.targets:
            return None
        weights = [t['Pf'] for t in self.targets]
        selected_list = random.choices(self.targets, weights=weights, k=1)
        return selected_list[0]

if __name__ == "__main__":
    report_folder = "/data/mingxuanzhu/SOLAR"

   

    selector = GapSmithSelector(report_folder)

    selector.parse_all_reports()
    selector.calculate_metrics()

    # 测试选择 5 次
    for i in range(1, 6):
        target = selector.select_next_target()
        if target:
            print(f"Iteration {i}:")
            print(f"  Selected File : {target['filename']}")
            print(f"  Pf Weight     : {target['Pf']:.6f}")
            print("-" * 60)
        else:
            print("No target selected.")
