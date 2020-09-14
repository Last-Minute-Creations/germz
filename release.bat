@echo off
mkdir game
copy build\germz.exe game
REM copy germz.info game
xcopy build\data game\data /e /i /h
mkdir game\s
echo germz.exe > game\s\startup-sequence
exe2adf data\germz.exe -l "GermZ" -a "germz.adf" -0 -d game
rmdir game /s /q
