import re
import time
import os
from pathlib import Path
from typing import Optional, Dict, Any

from openai import OpenAI


class UncoveredRequirementSummarizer:
    """
    Tool for summarizing uncovered requirements.

    Main responsibilities:
    - Call Chat Completions API to summarize uncovered requirements
    - Strip markdown code fences if present
    - Save summary results to output directory
    - Record generation results and failures
    """

    def __init__(
        self,
        api_key: str,
        base_url: str = "https://api.deepseek.com/v1",
        output_dir: str = "summaries",
        model: str = "deepseek-chat",
        temperature: float = 0.3,
        max_tokens: int = 8192,
    ):
        """
        :param api_key: API key for OpenAI
        :param base_url: Base URL for OpenAI
        :param output_dir: Directory to save uncovered requirement summaries
        :param model: Model name
        :param temperature: Sampling temperature
        :param max_tokens: Max tokens
        """
        if not api_key or not isinstance(api_key, str):
            raise ValueError("api_key must be a non-empty string")
        if not base_url or not isinstance(base_url, str):
            raise ValueError("base_url must be a non-empty string")
        if not model or not isinstance(model, str):
            raise ValueError("model must be a non-empty string")
        if temperature < 0:
            raise ValueError("temperature must be >= 0")
        if max_tokens <= 0:
            raise ValueError("max_tokens must be > 0")

        self.client = OpenAI(api_key=api_key, base_url=base_url)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.model = model
        self.temperature = temperature
        self.max_tokens = max_tokens

        # Used for removing markdown fences if model wraps output in ``` ```
        self.fence_pattern = re.compile(r"```(?:[a-zA-Z0-9_+-]+)?\s*\n(.*?)```", re.DOTALL)

        # Store summarization records
        self.results: list[Dict[str, Any]] = []
        self.processed_iterations: set[int] = set()

    def _strip_markdown_fence(self, text: str) -> str:
        """Remove markdown code fences if present; otherwise return stripped text."""
        if text is None:
            return ""
        m = self.fence_pattern.search(text)
        return (m.group(1) if m else text).strip()

    def _call_api(self, summary_prompt: str, retry_times: int = 3, backoff_base: float = 2.0) -> Optional[str]:
        """Call API with retries; return summary content or None."""
        if not summary_prompt or not isinstance(summary_prompt, str):
            raise ValueError("summary_prompt must be a non-empty string")
        if retry_times <= 0:
            raise ValueError("retry_times must be > 0")

        last_error: Optional[str] = None
        for attempt in range(retry_times):
            try:
                resp = self.client.chat.completions.create(
                    model=self.model,
                    messages=[{"role": "user", "content": summary_prompt}],
                    temperature=self.temperature,
                    max_tokens=self.max_tokens,
                )
                content = resp.choices[0].message.content
                return content
            except Exception as e:
                last_error = f"{type(e).__name__}: {e}"
                if attempt < retry_times - 1:
                    time.sleep(backoff_base ** attempt)

        return None

    def run(self, summary_prompt: str, iteration_index: int, retry_times: int = 3) -> Optional[str]:
        """
        Main execution function:
        - Call API to summarize uncovered requirements
        - Strip markdown fences
        - Save summary to file
        - Record result in self.results
        """
        if not isinstance(iteration_index, int) or iteration_index < 0:
            raise ValueError("iteration_index must be a non-negative int")

        duplicate = iteration_index in self.processed_iterations

        raw = self._call_api(summary_prompt=summary_prompt, retry_times=retry_times)

        if raw is None:
            record = {
                "iteration": iteration_index,
                "status": "failed",
                "duplicate_iteration": duplicate,
                "saved_path": None,
            }
            self.results.append(record)
            self.processed_iterations.add(iteration_index)
            print(f"[UncoveredRequirementSummarizer] summarization FAILED (iteration={iteration_index})")
            return None

        clean = self._strip_markdown_fence(raw)
        if len(clean) < 80 or "[Coverage Goal]" not in clean:
            clean = raw.strip()
        path = self.output_dir / f"uncovered_summary_{iteration_index}.txt"
        path.write_text(clean + "\n", encoding="utf-8")
        record = {
            "iteration": iteration_index,
            "status": "ok",
            "duplicate_iteration": duplicate,
            "saved_path": str(path),
            "chars": len(clean),
        }
        self.results.append(record)
        self.processed_iterations.add(iteration_index)     
        print(f"[UncoveredRequirementSummarizer] saved -> {path}")
        return clean
