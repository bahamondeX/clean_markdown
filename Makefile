.PHONY: all BUILD
# Setup Python virtual environment
build:
    cd clean_markdown
	python setup.py build_ext --inplace
	
