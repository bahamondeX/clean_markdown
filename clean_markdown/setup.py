# setup.py
from setuptools import setup, Extension

# Define the extension module
markdown_cleaner_module = Extension(
    'clean_markdown',
    sources=['clean_markdown.c'],
    include_dirs=[],  # Add any include directories if needed
    libraries=[],     # Add any libraries if needed
    extra_compile_args=['-O3']  # Optimize for performance
)

# Setup the package
setup(
    name='clean_markdown',
    version='0.1.0',
    description='Markdown cleaning utility that removes formatting and handles code blocks',
    author='Oscar Bahamonde',
    author_email='oscar.bahamonde.dev@example.com',
    ext_modules=[markdown_cleaner_module],
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Programming Language :: Python :: 3',
        'Programming Language :: C',
        'Topic :: Text Processing :: Markup'
    ],
)