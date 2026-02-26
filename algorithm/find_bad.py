import argparse
import json
import random
from pathlib import Path

class BadCaseFinder:
    """
    Tool for retrieving one bad case for a given target file.

    Expected directory layout:
        bad_cases/
            gcc__c-family__c-pretty-print.cc/
                case1.json
                case2.json
                ...
    """
    def __init__(self, bad_cases_dir: str = "bad_cases", seed: int | None = None):
        self.bad_cases_dir = Path(bad_cases_dir)

        if seed is not None:
            random.seed(seed)

    @staticmethod
    def normalize_target_name(target_file: str) -> str:
        """
        Normalize target file path to match folder name.
        Example:
            gcc/c-family/c-pretty-print.cc
        becomes:
            gcc__c-family__c-pretty-print.cc
        """
        return target_file.replace("\\", "/").replace("/", "__")

    def find_target_folder(self, target_file: str) -> Path:
        folder_name = self.normalize_target_name(target_file)
        folder = self.bad_cases_dir / folder_name

        if not folder.exists() or not folder.is_dir():
            raise FileNotFoundError(
                f"Folder not found for target '{target_file}'. "
                f"Expected: {folder}"
            )

        return folder

    def pick_random_json(self, folder: Path) -> Path:
        json_files = list(folder.glob("*.json"))

        if not json_files:
            raise FileNotFoundError(f"No JSON bad cases found in {folder}")

        return random.choice(json_files)

    @staticmethod
    def format_output(data: dict) -> str:
        """
        Format according to required output template.
        Expected JSON fields:
            - prompt_base
            - compilation_status
            - improved_other_files
            - improved_in_file
        """

        return (
            "Previous programs generated from the earlier prompt did not meet the coverage goal for this file.\n"
            f"{data.get('prompt_base', '')}\n"
            "Here is some information:\n"
            f"Compilation status:  {data.get('compilation_status', '')}\n"
            f"Coverage improvements in other files: {data.get('improved_other_files', '')}\n"
            "Coverage was improved in other parts of this file:\n"
            f"{data.get('improved_in_file', '')}\n"
            "This suggests that either (1) the stated requirements do not match the actual coverage targets, \n"
            "or (2) the generated program failed to trigger the relevant compilation options.\n"
        )


    def run(self, target_file: str) -> str:
        """
        Main execution:
        - Locate target folder
        - Randomly select one JSON bad case
        - Format output
        - Print and return formatted content
        """
        folder = self.find_target_folder(target_file)
        json_path = self.pick_random_json(folder)

        data = json.loads(json_path.read_text(encoding="utf-8"))
        formatted = self.format_output(data)

        print(f"[find_bad] target_file   : {target_file}")
        print(f"[find_bad] matched_folder: {folder}")
        print(f"[find_bad] selected_json : {json_path}")
        print("\n===== BAD CASE OUTPUT =====\n")
        print(formatted)

        return formatted


