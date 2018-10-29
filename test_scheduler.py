#!/bin/python

from shutil import copyfile
from subprocess import call
import time
import os

# be careful about changing these numbers
# or the drive file will run out of space
size = 2**16
number_of_tests = 1000

sched_file = open("/sys/block/hda/queue/scheduler", "w")
dir = "test_dir"
os.mkdir(dir)
os.chdir(dir)

print "Testing noop for baseline time:"
call(["echo", "noop"], stdout=sched_file)
call(["cat", "/sys/block/hda/queue/scheduler"])

src_file1 = open("src_file.txt", "w+")
src_file1.write(os.urandom(size))
src_file1.close()

print "Starting I/O with noop algorithm:"
benchmark = time.clock()

for x in range(number_of_tests):
	copyfile("./src_file.txt", "./dest_file_a{}.txt".format(x))
elapsed_time = time.clock() - benchmark
print "Elapsed time: " + str(elapsed_time)

print "Testing look for improved time:"
call(["echo", "look"], stdout=sched_file)
call(["cat", "/sys/block/hda/queue/scheduler"])

src_file2 = open("src_file2.txt", "w+")
src_file2.write(os.urandom(size))
src_file2.close()

print "Starting I/O with look algorithm"
benchmark = time.clock()

for x in range(number_of_tests):
	copyfile("./src_file.txt", "./dest_file_b{}.txt".format(x))
elapsed_time = time.clock() - benchmark
print "Elapsed time:" + str(elapsed_time)
sched_file.close()
os.chdir("..")
call(["rm", "-rf", "test_dir"])

