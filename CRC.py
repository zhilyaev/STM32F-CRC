def main (init_crc=0xFF, input_data=0xC1, msb=7, poly=0xCB):
crc = init_crc ^ input_data
i = 0
while (True):
print(hex(crc))
if (crc » msb == 1):
crc = ((crc « 1)& 0xff) ^ poly
print('[i]='+str(i))
else:
crc = crc « 1

crc = crc & 0xff
i = i + 1
#print('[i]='+str(i))
#if (not(i < len(input_data) * 8)):
if (i > 7):
break

return crc

print(hex(main()))

