from setuptools import setup, find_packages

with open('README.md', 'r') as fh:
    long_description = fh.read()

setup(
    name='qcrepocleaner',
    version='1.9',
    author='Barthelemy von Haller',
    author_email='bvonhall@cern.ch',
    url='https://gitlab.cern.ch/AliceO2Group/QualityControl/Framework/script/RepoCleaner',
    license='GPLv3',
    description='Set of tools to clean up the QCDB repository.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    packages=find_packages(),
    classifiers=[
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Programming Language :: Python :: 3',
        'Topic :: Utilities',
        'Environment :: Console',
        'Operating System :: Unix',
        'Development Status :: 5 - Production/Stable'
    ],
    python_requires='>=3.6',
    install_requires=['requests', 'dryable', 'responses', 'PyYAML', 'python-consul', 'psutil'],
    scripts=[
        'qcrepocleaner/o2-qc-repo-cleaner',
        'qcrepocleaner/o2-qc-repo-delete-objects-in-runs',
        'qcrepocleaner/o2-qc-repo-delete-not-in-runs',
        'qcrepocleaner/o2-qc-repo-delete-objects',
        'qcrepocleaner/o2-qc-repo-delete-time-interval',
        'qcrepocleaner/o2-qc-repo-find-objects-not-updated',
        'qcrepocleaner/o2-qc-repo-move-objects',
        'qcrepocleaner/o2-qc-repo-update-run-type'],
    include_package_data=True,
    package_data={
        'qcrepocleaner': ['qcrepocleaner/config.yaml']
    }
)
