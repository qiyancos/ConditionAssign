#! /usr/bin/env python
# -*- coding: gb18030 -*-
import xlrd
import os
import sys
import codecs
reload(sys)
sys.setdefaultencoding('gb18030')

if len(sys.argv) < 3:
    print(u'======== Excel转TXT工具 ========')
    print(u'每个sheet以sheet_name转换为一个GBK编码文本文件')
    print(u'Usage: excel2txt.exe <excel_file> <out_dir> [sep=TAB]')
    print(u'Params:')
    print(u'  - excel_file : 待转换的Excel文件')
    print(u'  - out_dir    : 输出目录')
    print(u'  - sep        : 列分隔符, 默认为TAB')
    print(u'================================')
    sys.exit(-1)

EXCEL_FILE = sys.argv[1]
OUTPUT_DIR = sys.argv[2]
SEP = '\t' if len(sys.argv) < 4 else sys.argv[3]
if not os.path.exists(OUTPUT_DIR): 
    os.makedirs(OUTPUT_DIR)

def convertOldToNew(condition):
    result = ""
    index = 0
    inString = 0
    changeList = ['>', '<', '%', ':', '!']
    while index < len(condition):
        result += condition[index]
        if inString == 1 and condition[index] == '\"':
            inString = 0
        elif inString == 0 and condition[index] == '\"':
            inString = 1
        elif inString == 2 and index + 1 < len(condition) and \
                condition[index + 1] == ';':
            result += '\"'
            inString = 0
        elif inString == 0:
            if condition[index] == '=' and \
                    condition[index - 1] not in changeList:
                if condition[index + 1] == '=':
                    index += 1
                result += '='
            elif condition[index] == '=' and condition[index - 1] == ':':
                result += '\"'
                inString = 2
        index += 1
    if inString == 2:
        result += '\"'
    return result

book = xlrd.open_workbook(EXCEL_FILE)
print('- excel file:', EXCEL_FILE)
print('- output dir:', OUTPUT_DIR)
print('- total sheets:', book.nsheets)
print('- start ...')
for i, sheet_name in enumerate(book.sheet_names()):
    sheet = book.sheet_by_name(sheet_name)
    print('- [{}/{}] {}, total rows: {}'.format(i+1, book.nsheets, \
            sheet_name, sheet.nrows))
    output_file = os.path.join(OUTPUT_DIR, sheet_name)
    with codecs.open(output_file, 'w', encoding='gb18030') as wf:
        for i in range(sheet.nrows):
            row = [str(x).strip() for x in sheet.row_values(i)]
            row[0] = convertOldToNew(row[0])
            wf.write('\t'.join(row) + '\n')
print('- done.')
