IP_VM2 := 1.1.1.1
IP_VM3 := 1.1.1.1
IP_VM4 := 1.1.1.1

lib-deploy-online:
	git clone http://github.com/sisoputnfrba/so-commons-library ../so-commons-library
	git clone http://github.com/sisoputnfrba/ansisop-parser ../ansisop-parser

lib-deploy-install:
	-cd ../so-commons-library && sudo $(MAKE) install
	-cd ../ansisop-parser/parser && $(MAKE) all && sudo $(MAKE) install

lib-deploy-all-online:
	$(MAKE) lib-deploy-online
	$(MAKE) lib-deploy-install

scp-all:
	-cd .. && scp -rpC ./repo utnso@$(IP_VM2):/home/utnso/
	-cd .. && scp -rpC ./so-commons-library utnso@$(IP_VM2):/home/utnso/
	-cd .. && scp -rpC ./ansisop-parser utnso@$(IP_VM2):/home/utnso/

	-cd .. && scp -rpC ./repo utnso@$(IP_VM3):/home/utnso/
	-cd .. && scp -rpC ./so-commons-library utnso@$(IP_VM3):/home/utnso/
	-cd .. && scp -rpC ./ansisop-parser utnso@$(IP_VM3):/home/utnso/

	-cd .. && scp -rpC ./repo utnso@$(IP_VM4):/home/utnso/
	-cd .. && scp -rpC ./so-commons-library utnso@$(IP_VM4):/home/utnso/
	-cd .. && scp -rpC ./ansisop-parser utnso@$(IP_VM4):/home/utnso/
	
repo-deploy-install:
	-cd ./programa && $(MAKE) programa
	-cd ./kernel && $(MAKE) kernel
	-cd ./umv && $(MAKE) umv
	-cd ./cpu && $(MAKE) cpu

all-deploy-install:
	$(MAKE) lib-deploy-install
	$(MAKE) repo-deploy-install
