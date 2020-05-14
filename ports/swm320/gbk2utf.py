#! python3
import os

for dirpath, dirnames, filenames in os.walk('chip'):
	for filename in filenames:
		filename = f'{dirpath}\\{filename}'

		try:
			text = open(filename, 'r', encoding='gbk').read()

		except Exception as e:
			print(f'{filename:36s} open fail')

		else:
			open(filename, 'w', encoding='utf-8').write(text)
			
			print(f'{filename:36s} ok')
