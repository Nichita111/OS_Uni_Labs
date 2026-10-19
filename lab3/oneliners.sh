# Lab 3, Part 4 - one-liners over access.log
# Run inside ~/os-lab3 with: bash oneliners.sh
# Each line of access.log is: date time user result page

# Q1. How many requests failed?
grep -c FAIL access.log

# Q2. How many different pages were requested, and which?
cut -d' ' -f5 access.log | sort -u | nl

# Q3. How many requests did each page get?
cut -d' ' -f5 access.log | sort | uniq -c

# Q4. Which user or users have the most failed requests, and how many?
grep FAIL access.log | cut -d' ' -f3 | sort | uniq -c | sort -rn | awk 'NR==1{max=$1} $1==max'

# Q5. How many requests did user3 make, and how many of them failed?
echo "user3: $(grep -cw user3 access.log) requests, $(grep -w user3 access.log | grep -c FAIL) failed"

# Q6. Print the last 3 failed requests, showing only time and user.
grep FAIL access.log | tail -3 | cut -d' ' -f2,3

# Q7. Which login shells appear in /etc/passwd, and how many accounts use each? (field 7, separator :)
cut -d: -f7 /etc/passwd | sort | uniq -c | sort -rn

# Q8. Why do ls /etc | wc -l and ls -l /etc | wc -l differ by exactly one?
echo "ls: $(ls /etc | wc -l) lines, ls -l: $(ls -l /etc | wc -l) lines, extra line: $(ls -l /etc | head -1)"

# Bonus. Lines where the minute ends in 0 and the request failed, and how many there are.
grep -E '^[0-9-]+ [0-9]{2}:[0-5]0 user[0-9]+ FAIL ' access.log
grep -cE '^[0-9-]+ [0-9]{2}:[0-5]0 user[0-9]+ FAIL ' access.log
