import os
import xml.etree.ElementTree as ET

def compare_xml_files(file1, file2):
  tree1 = ET.parse(file1)
  tree2 = ET.parse(file2)
  root1 = tree1.getroot()
  root2 = tree2.getroot()
  
  return compare_xml_elements(root1, root2)

def compare_xml_elements(elem1, elem2, path="/"):
  if elem1.tag != elem2.tag:
    print(f"Tags do not match at {path}: {elem1.tag} != {elem2.tag}")
    return False
  if elem1.text != elem2.text:
    print(f"Text content does not match at {path}: '{elem1.text}' != '{elem2.text}'")
    return False
  if elem1.tail != elem2.tail:
    print(f"Tail content does not match at {path}: '{elem1.tail}' != '{elem2.tail}'")
    return False
  if elem1.attrib != elem2.attrib:
    print(f"Attributes do not match at {path}: {elem1.attrib} != {elem2.attrib}")
    return False

  children1 = list(elem1)
  children2 = list(elem2)

  if len(children1) != len(children2):
    print(f"Number of children does not match at {path}: {len(children1)} != {len(children2)}")
    return False

  for i, (child1, child2) in enumerate(zip(children1, children2)):
    if not compare_xml_elements(child1, child2, path=f"{path}/{child1.tag}[{i + 1}]"):
      return False

  return True

files = [file[:file.find(".fmj")] for file in os.listdir('.') if file.endswith('.fmj')]
for file in files:
  file1 = file+'.2.ast'
  file2 = file+'.2.debug.ast'
  if os.path.exists(file1) and os.path.exists(file2):
    same = compare_xml_files(file1, file2)
    print(f'{file}: {same}')
