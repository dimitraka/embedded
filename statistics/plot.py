#! usr/local/bin/python

#from statistics import mean, median, stdev
from matplotlib import pyplot as plt

file = open("connection_duration.txt", "r")
pack_times = []
for val in file.read().split():
    pack_times.append(float(val))
file.close()
'''
mean_value = mean(pack_times)
min_value = min(pack_times)
max_value = max(pack_times)
median_value = median(pack_times)
std_dev = stdev(pack_times)

print("Mean value : %f \nMin value : %f\nMax value : %f\nMedian value : %f"
      % (mean_value, min_value, max_value, median_value))
print("Standard deviation : %f" % std_dev)
'''
plt.scatter(list(range(0, len(pack_times))), pack_times)
plt.xlabel('No of message')
plt.ylabel('Duration of each connection in seconds')
'''
plt.text(1, min_value+420, "Duration: 7200 sec\nInterval: 0.1 sec\nBMean value: %f us\nMin value: %f us\nMax value: %f us\nMedian value: %f us"
        % (mean_value, min_value, max_value, median_value),
        bbox=dict(facecolor='green', alpha=0.1))'''

plt.show()
