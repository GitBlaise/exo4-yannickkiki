import argparse

def parse_cmdline():
    parser = argparse.ArgumentParser(description='Encode-Decode')
    parser.add_argument('-d', '--decode', help='Decode input file')
    parser.add_argument('-e', '--encode', help='Encode input file')
    return parser.parse_args()

def bytes_from_file(filename):
	'''This function yield bytes of *filename*'''
	with open(filename, 'rb') as f:
	    while True:
	        byte = f.read(1)
	        if byte:
	        	yield byte
	       	else:
	            break

def encode_with_complementarity(byte):
	'''This function take a byte, do the "Not" operation on it and return the
	result'''
	return 	bytes([~byte[0] & 0xFF])

def write_in_file(prefix,input_file):
	'''This function read the input file byte by byte and for each byte read,
	he encode it and write the result in prefix_input_file''' 
	with open (prefix+input_file, 'wb') as f:
		for byte in bytes_from_file(input_file):
			byte_encoded=encode_with_complementarity(byte)
			f.write(byte_encoded)

if __name__ == "__main__":
	args= parse_cmdline()
	if args.decode and args.encode:
		print('Not both options at the same time')
	else:
		if args.encode:
			write_in_file('enc_',args.encode)
		elif args.decode:
			write_in_file('dec_',args.decode)
		else:
			print("Veuillez préciser les options, -h pour voir le message " 
				"d'aide")		