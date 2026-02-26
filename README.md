# GapSmith
## algorithm
- `algorithm/collect.py` - Tool for collecting coverage data from target directories
- `algorithm/uncovered_analyzer.py` - Tool for analyzing uncovered block
- `algorithm/sort.py` - Tool for selecting target file
- `algorithm/find_bad.py` -  Tool for retrieving one bad case for a given target file
- `algorithm/summarize.py` - Tool for summarizing uncovered requirements
- `algorithm/generate.py` - Tool for generating test program

## results
- `cold_start_results.json` - Results of the Cold-Start
- `coverage_improvement_results.json` - Results of the Coverage Improvement Experiment
- `whitefox_comparison_results.json` - Overall Comparison Results with WhiteFox
- `whitefox_case_results.json` - Case-by-Case Results with WhiteFox
- `ablation_study_results.json` - Results of the Ablation Study
- `cost_analysis_results.json` - Model Cost Analysis Results
- `base_model_comparison_results.json` - Base Model Comparison Results
- `failure_results.json` - Failure Statistics Results

## `main.py` - entrance of GapSmith
Run command: DEEPSEEK_API_KEY="xx" python main.py
Remember change your path for running

## Data storage folders
- `programs` - test program storage folder
- `prompts` - prompt storage folder
- `summaries` - summarization storage folder
- `bad_cases` - bad case storage folder