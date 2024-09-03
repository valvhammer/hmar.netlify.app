set commitmsg=%1
git add *
git commit -m "%commitmsg%"
git push -u origin master