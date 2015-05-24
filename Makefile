#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
all:
	@$(MAKE) -C libSimpleRSX --no-print-directory

clean:
	@$(MAKE) -C libSimpleRSX clean --no-print-directory
	@rm -rf *~
