for /f "tokens=*" %%a in ('git rev-parse --abbrev-ref HEAD') do set BRANCH_NAME=%%a
git fetch --all
git reset --hard origin/%BRANCH_NAME%
git pull
pause