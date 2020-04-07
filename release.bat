@echo off
mkdir game
copy germz game
REM copy aminer.info game
xcopy data game\data /e /i /h
mkdir game\s
echo aminer > game\s\startup-sequence
exe2adf germz -l "GermZ" -a "germz.adf" -0 -d game
rmdir game /s /q