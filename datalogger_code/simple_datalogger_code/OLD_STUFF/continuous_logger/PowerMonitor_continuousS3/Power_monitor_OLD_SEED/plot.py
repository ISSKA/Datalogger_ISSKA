#This program plots the result of the INA219
#Run from terminal with python plot.py in the right directory

import matplotlib.pyplot as plt
import numpy as np

#file to plot
file = "exemple.asc"

f = open(file,'r')
lines = f.readlines()

time = np.zeros(len(lines)-2) #-2 get rid of first and last line
current = np.zeros(len(lines)-2)

index = 0

for l in lines[1:-1]:
	time[index] = float(l.split(' ')[0])/1000.
	current[index] = float(l.split(' ')[4])
	if current[index] == -0.1:
		current[index] = 0.
	#print time[index], ' ', current[index]
	index += 1
f.close()

time -= time[0]

#avg_cons = sum(current)/current.size
avg_cons = 0
for i in range(current.size-1):
	avg_cons += current[i]*(time[i+1]-time[i])
avg_cons /= time[-2]

plt.figure(1)
plt.plot(time,current)
plt.xlabel('Time(s)')
plt.ylabel('Current (mA)')
plt.xlim = [time[0],time[-1]]
plt.text(0.78*plt.gca().get_xlim()[1],0.9*plt.gca().get_ylim()[1],
    'Average current: ' +str(avg_cons)[0:6]+' (mA)', horizontalalignment='center',verticalalignment='center')
	 
plt.show()