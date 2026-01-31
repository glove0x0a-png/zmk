cd "C:\Users\glove\OneDrive\Desktop\PicoW\_BB_ZMK\zmk-main\zmk"
set DATE_=%date: =0%
set TIME_=%time: =0%
set TIMESTAMP=%DATE_:~0,4%%DATE_:~5,2%%DATE_:~8,2%_%TIME_:~0,2%%TIME_:~3,2%

git init
git remote add origin https://github.com/glove0x0a-png/zmk.git
git remote -v

git add .
git commit -m "test"
git push -u origin main

ping localhost -n 1

echo.>>C:\Users\glove\OneDrive\Desktop\PicoW\_BB_ZMK\zmk-main\zmk-config\config\boards\shields\glove0x0a\glove0x0a.overlay
cd "C:\Users\glove\OneDrive\Desktop\PicoW\_BB_ZMK\zmk-main\zmk-config"
git init
git remote add origin https://github.com/glove0x0a-png/zmk-config.git
git remote -v


git add .
git commit -m "%TIMESTAMP%_releace"
git push -u origin master
pause