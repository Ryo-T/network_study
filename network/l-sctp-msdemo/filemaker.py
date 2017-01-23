#!/usr/bin/python
# coding: UTF-8
 
f = open('sample2.txt', 'w') # 書き込みモードで開く

for line in range(0,10000):
	buf = '%010d'%line
	f.write("%s\n"%buf) # 引数の文字列をファイルに書き込む

f.close() # ファイルを閉じる
