import os
import re
import time
from datetime import datetime
from pathlib import Path
from typing import Optional, Dict, Tuple
from concurrent.futures import ThreadPoolExecutor, as_completed
from threading import Lock

from openai import OpenAI


class BatchCodeGenerator:
    """
    Batch C program generator.
    Each batch creates a timestamped folder:
        programs/
            batch_YYYYMMDD_HHMMSS/
                program_0001.c
                program_0002.c
    """
    def __init__(
        self,
        api_key: str,
        base_url: str = "https://api.deepseek.com/v1",
        output_dir: str = "programs",
        model: str = "deepseek-chat",
        max_workers: int = 10,
        temperature: float = 0.8,
        max_tokens: int = 8192,
    ):
        self.client = OpenAI(api_key=api_key, base_url=base_url)
        self.base_output_dir = Path(output_dir)
        self.base_output_dir.mkdir(parents=True, exist_ok=True)
        self.batch_output_dir = None
        self.model = model
        self.max_workers = max_workers
        self.temperature = temperature
        self.max_tokens = max_tokens
        self.lock = Lock()
        self.generated_count = 0
        self.failed_count = 0
        self.fence_pattern = re.compile(
            r"```(?:[a-zA-Z0-9_+-]+)?\s*\n(.*?)```",
            re.DOTALL
        )
        self.include_to_end_pattern = re.compile(
            r"(?s)(#include[^\n]*\n.*\n\})\s*$"
        )

    def create_batch_folder(self) -> Path:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        batch_dir = self.base_output_dir / f"batch_{timestamp}"
        batch_dir.mkdir(parents=True, exist_ok=True)
        self.batch_output_dir = batch_dir
        print(f"[Batch] Created folder: {batch_dir}")
        return batch_dir

    def generate_code(self, prompt: str, retry_times: int = 3) -> Optional[str]:
        for attempt in range(retry_times):
            try:
                resp = self.client.chat.completions.create(
                    model=self.model,
                    messages=[{"role": "user", "content": prompt}],
                    temperature=self.temperature,
                    max_tokens=self.max_tokens,
                )
                return resp.choices[0].message.content
            except Exception:
                if attempt < retry_times - 1:
                    time.sleep(2 ** attempt)
        return None


    def strip_markdown_fence(self, text: str) -> str:
        if text is None:
            return ""
        m = self.fence_pattern.search(text)
        return (m.group(1) if m else text).strip()


    def extract_c_program(self, text: str) -> str:
        s = self.strip_markdown_fence(text)
        s = s.replace("\r\n", "\n").replace("\r", "\n").strip()
        m = self.include_to_end_pattern.search(s)
        if m:
            return m.group(1).strip() + "\n"
        return s + "\n"

    def save_program(self, code: str, index: int, prefix: str) -> Path:
        filename = self.batch_output_dir / f"{prefix}_{index:04d}.c"
        filename.write_text(code, encoding="utf-8")
        return filename


    def _generate_one_program(self, index, base_prompt, prefix):
        raw = self.generate_code(base_prompt)
        if not raw:
            with self.lock:
                self.failed_count += 1
            return index, False, "empty"
        program = self.extract_c_program(raw)
        path = self.save_program(program, index, prefix)
        with self.lock:
            self.generated_count += 1
        return index, True, str(path)

    def generate_batch(
        self,
        batch_size=50,
        base_prompt=None,
        prefix="program"
    ):
        if base_prompt is None:
            base_prompt = (
                "Generate ONE ISO C99 program.\n"
                "Must include main.\n"
                "Print checksum.\n"
                "Output only code."
            )
        self.create_batch_folder()
        self.generated_count = 0
        self.failed_count = 0
        start = time.time()
        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            futures = [
                executor.submit(
                    self._generate_one_program,
                    i,
                    base_prompt,
                    prefix
                )
                for i in range(1, batch_size + 1)
            ]
            done = 0
            for f in as_completed(futures):
                index, success, info = f.result()
                done += 1
                print(f"[{done}/{batch_size}] {info}")
        elapsed = time.time() - start
        return {

            "batch_dir": str(self.batch_output_dir),
            "generated": self.generated_count,
            "failed": self.failed_count,
            "elapsed": elapsed
        }
