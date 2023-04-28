if [ "$1" -eq "1" ]
then
    git remote set-url origin https://github.com/Saydemr/parallel-programming-tum.git
else
    git remote set-url origin git@gitlab.lrz.de:lrr-tum/teaching/parprog/ss2023/published-assignments.git
    git pull
fi

