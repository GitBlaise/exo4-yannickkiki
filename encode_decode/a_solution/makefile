CFLAGS = -Wall -pedantic -O3
CC = gcc
APP = codec
SHASUM_APP = codec_s

$(APP): codec.c
	gcc $(CFLAGS) -o $@ $^

$(SHASUM_APP): codec.c
	gcc $(CFLAGS) -DSHASUM -o $@ $^

shasum: $(SHASUM_APP)
help:
	@echo "make help: Display this help"
	@echo "make: Build"
	@echo -n "make shasum: Build a version with strict password "
	@echo "\"encryption\" (Linux only)"
	@echo "make clean: Clean"
	@echo

.PHONY: clean
clean:
	rm -f $(APP) $(SHASUM_APP)
