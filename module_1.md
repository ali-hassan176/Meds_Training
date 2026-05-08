# Module 1
## Day 1— Linux Fundamentals & Shell Navigation
### Exercise
#### Question no 1:
```bash
ali@ali-ThinkBook-14-G6-IRL:~/meds/mkdir -p project/{src/{rtl,tb,include},docs,scripts,build}
ali@ali-ThinkBook-14-G6-IRL:~/meds/project$ tree
.
├── build
├── docs
├── scripts
└── src
    ├── include
    ├── rtl
    └── tb

7 directories, 0 files

```
#### Question no 2:
```bash
ali@ali-ThinkBook-14-G6-IRL:~/meds/riscv/wget https://github.com/riscv/riscv-isa-manual.git

file riscv-isa-manual.git 

output

riscv-isa-manual.git: HTML document, Unicode text, UTF-8 text, with very long lines (25419)

ls -lh

output

total 296K
-rw-rw-r-- 1 ali ali 296K اپریل  29 19:18 riscv-isa-manual.git

```
#### Question no 3:
```bash
for i in {1..100};do echo $RANDOM;done > data.txt
# it generate 100 random variables and write them into data.txt
cat data.txt
sort data.txt | uniq -c | sort -nr | head -n 10 > top10.txt
#sort is used to sort 100 random variables and uniq count the unqique elements and sort -nr again sort them and head - n 10 select the top 10 and write them in new top10.txt file
cat top.txt

```
#### Question no 4:
```bash
ssh -T git@github.com

#output
Hi ali-hassan176! You've successfully authenticated, but GitHub does not provide shell access.
```
![SSH success](./day1/images/ssh.png)
#### Question no 5:
```bash
find /usr/include . -name '*.c' -type f | xargs wc -l | sort -nr | head -n 5
# find navigate to the usr/include and find all the files with .c extension and -type f is for confirming the file type and xargs passed the output to wc -l that counts the occurance and sort -nr sort them and head -n 5 give the top 5 files
```

## Day 2—Shell Scripting & Automation
### Exercise
#### Question no 1:
##### Organizer.sh
```bash
#!/bin/bash

DIR=$1

if [ ! -d "$DIR" ]; then
    echo "Invalid directory"
    exit 1
fi

mkdir -p "$DIR/verilog" "$DIR/c_code" "$DIR/docs"

for file in "$DIR"/*; do
    [ -f "$file" ] || continue

    ext="${file##*.}"

    case "$ext" in
        sv) mv "$file" "$DIR/verilog/" ;;
        c) mv "$file" "$DIR/c_code/" ;;
        txt) mv "$file" "$DIR/docs/" ;;
    esac
done

echo "Organization complete"

```
#### Question no 2:
##### file_stats.sh
```bash
#!/bin/bash

dir=$1

if [ ! -d "$dir" ]; then
    echo "Invalid directory"
    exit 1
fi

echo "Total files: $(find "$dir" -type f | wc -l)"
echo "Total directories: $(find "$dir" -type d | wc -l)"

echo "Largest file:"
find "$dir" -type f -exec du -h {} + | sort -rh | head -n 1

echo "Most recent file:"
find "$dir" -type f -printf "%T@ %p\n" | sort -n | tail -1

```
#### Question no 3:
##### sim_checker.sh
```bash
#!/bin/bash

log=$1

errors=$(grep -c "ERROR" "$log")
warnings=$(grep -c "WARNING" "$log")
passes=$(grep -c "PASS" "$log")

echo "ERRORS: $errors"
echo "WARNINGS: $warnings"
echo "PASSES: $passes"

if [ "$errors" -gt 0 ]; then
    exit 1
fi

exit 0

```
#### Question no 4:
##### batch_rename.sh
```bash
#!/bin/bash

pre=$1
suf=$2
dir=$3

if [ $# -ne 3 ]; then
    echo "Usage: $0 <prefix> <suffix> <directory>"
    exit 1
fi

if [ ! -d "$dir" ]; then
    echo "Error: Directory does not exist"
    exit 1
fi
for file in "$dir"/*; do
    [ -f "$file" ] || continue

    base=$(basename "$file")
    if [[ "$base" =~ ^${pre}_old_([0-9]+)\.sv$ ]]; then

        num="${BASH_REMATCH[1]}"
        new_name="${suf}_new_${num}.sv"

        mv "$file" "$dir/$new_name"
    fi
done

echo "Renaming completed successfully"

```
## Day 3— Git Fundamentals & Version Control
### Exercise
#### Question no 1:
```bash 
git log --oneline --graph
* 9d96e84 (HEAD -> main) Update README with progress
* b36f91c Add day 3 git notes
* 221cf6d Add day 2 bash scripting notes
* b819aaa Add day1 Linux notes
* 185a566 Add README with student info

```
#### Question no 2:

#### Question no 3:
#### Question no 4:
#### Question no 5:
#### Question no 6:

## Day 4—Advanced Git: Branching, Merging & Collaboration
### Exercise
#### Question no 1:

#### Question no 2:
#### Question no 3:
#### Question no 4:
#### Question no 5:
#### Question no 6:
## Day 5—Build Systems (Makefiles) & Grand Assignment
### Exercise
#### Question no 1:

#### Question no 2:
#### Question no 3:
## Grand Assignment: Module 1 capstone