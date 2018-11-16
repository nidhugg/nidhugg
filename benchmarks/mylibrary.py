#!/usr/bin/python3

# Copyright (C) 2018
# Library to generate tables

import math


def get_nice_time(runningtime):
	real_time = float(runningtime)
	if (real_time >= 60):
		min_num = int(real_time / 60)
		second_num = int(real_time - min_num * 60)
		return str(min_num)+"m"+str(second_num)+'s'
	else:
		return "{0:0.2f}".format(second_num)+'s'

def get_nice_tracecount(tracecount):
	real_tracecount = int(tracecount)
	ret  = ""	
	while (1):
		if real_tracecount >= 1000:			
			keep = int(real_tracecount / 1000)
			if ((real_tracecount - 1000 * keep) >= 100):
				ret =  str((real_tracecount - 1000 * keep)) + ' ' + ret
			elif ((real_tracecount - 1000 * keep) >= 10):
				ret = '0'+str((real_tracecount - 1000 * keep)) + ' ' + ret
			else: 
				ret = '00'+str((real_tracecount - 1000 * keep)) + ' ' + ret
			real_tracecount = keep
		else:
			ret = str(real_tracecount) + ' ' + ret
			return ret.strip()   
	

def get_nice_etime(etime):
	return "{0:0.2f}".format(etime)+'ms'

def get_result(tst, file, tool):
	with open(file,'r') as content_file:
		content = content_file.read()
	
	tstResults = content.split("<")
	found = 0
	
	for test in tstResults:		
		if ((tst['tstname'] in test) and (tool in test)):
			found = 1
			if ("Trace count:" in test):
				lines = test.split("\n")
				for line in lines:
					if ("Trace count:" in line):
						tst[tool+' execs'] = get_nice_tracecount(str(line.split(":")[1].split()[0]))
					else:
						if "real" in line:
							r_time = line.split()[1]
							r_min = r_time.split("m")[0]
							r_second = r_time.split("m")[1].split("s")[0]
							tst[tool+' time'] = r_min+"m"+"{0:0.2f}s".format(float(r_second))
					
			else:
				if ("Killed" in test):
					tst[tool+' execs'] = "t/o"
					tst[tool+' time'] = "t/o"
				else:
					tst[tool+' execs'] = "error"
					tst[tool+' time'] = "error"
			break

	if (found == 0):
		tst[tool+' execs'] = "n/a"
		tst[tool+' time'] = "n/a"	
