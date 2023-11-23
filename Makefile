all:
	cd src && $(MAKE)

clean:
	rm -f ./bql
	cd src && $(MAKE) clean