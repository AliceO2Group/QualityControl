Here are the tools to clean up the CCDB of the QC.

## Entry point
It is `repoCleaner.py`. See the long comment at the beginning.

## Usage
```
./repoCleaner [--dry-run] [--log-level 10] [--config config.yaml]
```

## Config
The file `config.yaml` contains the rules to be followed to clean up the database.
It also contains the CCDB url.

## Test
`cd QualityControl/Framework/script/RepoCleaner ; python3 -m unittest discover`