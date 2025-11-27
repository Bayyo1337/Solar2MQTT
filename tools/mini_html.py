Import("env")
import os
import glob
from pathlib import Path
import sys
import subprocess

def install(package):
    subprocess.check_call([sys.executable, "-m", "pip", "install", package])

try:
    import minify_html
except ImportError:
    install('minify_html')
    import minify_html

filePath = 'src/webpages/'
try:
  print("==========================")
  print("Generating webpage")
  print("==========================")
  print("Preparing html.h file from source")
  print("  -insert header") 
  cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
  print("  -insert html")

  for x in glob.glob(filePath+"*.html"):
   print("prozessing file:" + Path(x).stem)
   print(Path(x).stem)
   cpp_output += "static const char "+Path(x).stem+"[] PROGMEM = R\"rawliteral("
   f = open(x, "r")
   if env.GetProjectOption("build_type") == "debug":
        cpp_output += f.read()  
   else:
      #cpp_output += f.read()  #disable compression until fixed that the compressor remove %VARIABLE%
       cpp_output += minify_html.minify(f.read(), minify_js=True)

   f.close()
   cpp_output += ")rawliteral\";\n"

   f = open ("./src/html.h", "w")
   f.write(cpp_output)
   f.close()
   print("==========================\n")

except SyntaxError as e:
  print(e)
