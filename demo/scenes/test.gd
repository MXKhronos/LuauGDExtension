extends Node2D

func _ready():
	# 1. Print the maximum possible 64-bit signed integer
	# This equals 9,223,372,036,854,775,807
	var max_64bit_int = 0x7fffffffffffffff
	print("Max 64-bit int value: ", max_64bit_int, " Is64-Bit: ", max_64bit_int == 9223372036854775807)
	
	# 2. Trigger an intentional overflow
	# A 64-bit int wraps to a negative number when you add 1 to its max value
	var overflow_test = max_64bit_int + 1
	print("Overflow result (should be negative): ", overflow_test)
