
import os, osproc, strutils, re

# FIXME: Add command line

for file in walkDirRec "../build/driver":
  let outp_shell = execProcess("file " & file)
  if contains(outp_shell, ": ELF") and contains(outp_shell, "executable"):
    # FIXME: COPY TO TMP FOLDER
    echo file

# FIXME: Create tar archive
