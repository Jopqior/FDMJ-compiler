import os

def compare_files(file1, file2):
  with open(file1, 'r') as f1, open(file2, 'r') as f2:
    lines1 = [line.strip() for line in f1 if line.strip()]
    lines2 = [line.strip() for line in f2 if line.strip()]
    return lines1 == lines2

files = [file[:file.find(".fmj")] for file in os.listdir('.') if file.endswith('.fmj')]
for file in files:
  file1 = file+'.1.src'
  file2 = file+'.1.debug.src'
  if os.path.exists(file1) and os.path.exists(file2):
    same = compare_files(file1, file2)
    print(f'{file}: {same}')

