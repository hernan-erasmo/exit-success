online-lib-install:
	git clone http://github.com/sisoputnfrba/so-commons-library ./so-commons-library
	cd ./so-commons-library
	sudo make install

	git clone http://github.com/sisoputnfrba/ansisop-parser ./ansisop-parser
	cd ./ansisop-parser/parser 
	make all
	sudo make install
	