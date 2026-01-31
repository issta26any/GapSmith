import os
import re
from collections import OrderedDict

class UncoveredBlockAnalyzer:
    def __init__(self, gcov_file):
        self.gcov_file = gcov_file
        self.uncovered_blocks = [] 
        self.line_pattern = re.compile(r'^\s*(?P<count>[#\-\d]+):\s*(?P<line_num>\d+):(?P<code>.*)$')
    
    def parse_gcov_file(self):
        if not os.path.isfile(self.gcov_file):
            raise FileNotFoundError(f"file not found: {self.gcov_file}")
        
        current_block = []  
        start_line = None    
        
        with open(self.gcov_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                match = self.line_pattern.match(line)
                if not match:
                    continue
                
                count = match.group('count').strip()
                line_num = int(match.group('line_num'))
                code = match.group('code')
            
                if count == '#####':

                    if not current_block:
                        start_line = line_num
                    current_block.append({
                        'line_num': line_num,
                        'code': code
                    })
                else:
    
                    if current_block:
                        self._save_block(start_line, current_block)
                        current_block = []
                        start_line = None
            

            if current_block:
                self._save_block(start_line, current_block)
    
    def _save_block(self, start_line, block):

        non_empty_lines = [
            line for line in block 
            if line['code'].strip() and not self._is_pure_comment(line['code'])
        ]
        
        if non_empty_lines:
            end_line = block[-1]['line_num']
            self.uncovered_blocks.append({
                'start_line': start_line,
                'end_line': end_line,
                'block_size': len(block),
                'code_lines': block
            })
    
    def _is_pure_comment(self, code):

        stripped = code.strip()
        # C/C++ 注释判断
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            return True
        return False
    
    def sort_blocks_by_size(self):

        self.uncovered_blocks.sort(key=lambda x: x['block_size'], reverse=True)
    
    def get_blocks(self):
        
        return self.uncovered_blocks
    
    def print_blocks(self, max_blocks=None, min_size=1):
        
        filtered_blocks = [b for b in self.uncovered_blocks if b['block_size'] >= min_size]
        blocks_to_print = filtered_blocks[:max_blocks] if max_blocks else filtered_blocks
        
        print(f"\n{'='*80}")
        print(f"file: {os.path.basename(self.gcov_file)}")
        print(f"total {len(filtered_blocks)} uncovered blocks (>= {min_size} lines)")
        print(f"{'='*80}\n")
        
        for idx, block in enumerate(blocks_to_print, 1):
            self._print_single_block(idx, block)
    
    def _print_single_block(self, idx, block):
        start = block['start_line']
        end = block['end_line']
        size = block['block_size']
        
        print(f"[Block {idx}] 行 {start}-{end} (共 {size} 行)")
        print("-" * 80)
        
        for line_info in block['code_lines']:
            line_num = line_info['line_num']
            code = line_info['code']
            print(f"#####: {line_num:5d}:{code}")
        
        print()
    
    def export_to_file(self, output_file, min_size=1):
        filtered_blocks = [b for b in self.uncovered_blocks if b['block_size'] >= min_size]
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(f"{'='*80}\n")
            f.write(f"file: {os.path.basename(self.gcov_file)}\n")
            f.write(f"total {len(filtered_blocks)} uncovered blocks (>= {min_size} lines)\n")
            f.write(f"{'='*80}\n\n")
            
            for idx, block in enumerate(filtered_blocks, 1):
                start = block['start_line']
                end = block['end_line']
                size = block['block_size']
                
                f.write(f"[Block {idx}] 行 {start}-{end} (共 {size} 行)\n")
                f.write("-" * 80 + "\n")
                
                for line_info in block['code_lines']:
                    line_num = line_info['line_num']
                    code = line_info['code']
                    f.write(f"#####: {line_num:5d}:{code}\n")
                
                f.write("\n")
        
        print(f"result saved to: {output_file}")
    
    def get_statistics(self):
        if not self.uncovered_blocks:
            return {
                'total_blocks': 0,
                'total_lines': 0,
                'max_block_size': 0,
                'avg_block_size': 0
            }
        
        total_lines = sum(b['block_size'] for b in self.uncovered_blocks)
        max_size = max(b['block_size'] for b in self.uncovered_blocks)
        avg_size = total_lines / len(self.uncovered_blocks)
        
        return {
            'total_blocks': len(self.uncovered_blocks),
            'total_lines': total_lines,
            'max_block_size': max_size,
            'avg_block_size': avg_size
        }
    
    def print_statistics(self):
        stats = self.get_statistics()
        print(f"\nstatistics:")
        print(f"  total uncovered blocks: {stats['total_blocks']}")
        print(f"  total uncovered lines: {stats['total_lines']}")
        print(f"  max block size: {stats['max_block_size']} lines")
        print(f"  avg block size: {stats['avg_block_size']:.2f} lines")
    
    def get_single_block(self, block_index=0):
        if not self.uncovered_blocks:
            return None
        
        if block_index >= len(self.uncovered_blocks):
            return None
        
        block = self.uncovered_blocks[block_index]

        code_lines = []
        for line_info in block['code_lines']:
            code_lines.append(line_info['code'])
        
        return {
            'start_line': block['start_line'],
            'end_line': block['end_line'],
            'block_size': block['block_size'],
            'code_text': '\n'.join(code_lines)
        }
    
    def print_single_block_clean(self, block_index=0):
        block_data = self.get_single_block(block_index)
        
        if not block_data:
            print(f"block index {block_index} not found")
            return
        
        print(f"\n{'='*80}")
        print(f"Block {block_index + 1}: line {block_data['start_line']}-{block_data['end_line']} (total {block_data['block_size']} lines)")
        print(f"{'='*80}")
        print(block_data['code_text'])
        print(f"{'='*80}\n")
    
    def export_single_block_clean(self, output_file, block_index=0):
        block_data = self.get_single_block(block_index)
        
        if not block_data:
            print(f"block index {block_index} not found")
            return
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(f"// Block {block_index + 1}: 行 {block_data['start_line']}-{block_data['end_line']} (共 {block_data['block_size']} 行)\n")
            f.write(f"// File: {os.path.basename(self.gcov_file)}\n\n")
            f.write(block_data['code_text'])
            f.write("\n")
        
        print(f"block saved to: {output_file}")
        print(f"line {block_data['start_line']}-{block_data['end_line']} (total {block_data['block_size']} lines)")

