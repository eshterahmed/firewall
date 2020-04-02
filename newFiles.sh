IFS=$'\n'
for e in *; 
do
    if ! egrep -q "$e$" checksums.txt; then # if a file in directory is not in checksums.txt
    if [ $e != "checksums.txt" ]; then # as long as the file isn't checksums.txt
    md5sum $e >> checksums.txt; # add the file to checksums.txt
    fi
    fi
done

IFS=$'\n'       
set -f          
for i in $(cat < checksums.txt); do
    a=$(echo $i | awk '{print $2}')
    if ! [ -a $a ]; then # if a file in checksums.txt is no longer in the directory
    echo "$a not found"
    sed -i "/$a/d" checksums.txt # delete file from checksums.txt
    echo "$a deleted from checksums.txt"
    fi    
done






