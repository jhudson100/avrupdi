
check:
	@mypy $(wildcard *.py)

dist: clean
	python -m zipapp . -o ../avrupdi.pyz


clean:
	-/bin/rm -r __pycache__ .mypy_cache
