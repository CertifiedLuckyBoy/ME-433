# HW9 DSP

The sample data and instructor sample code were downloaded from the HW9 `dsp` folder.

## Sample Rates

| file | points | total time [s] | sample rate [Hz] |
| --- | ---: | ---: | ---: |
| sigA.csv | 50000 | 4.9999 | 10000.20 |
| sigB.csv | 16500 | 4.9997 | 3300.20 |
| sigC.csv | 20000 | 7.9996 | 2500.13 |
| sigD.csv | 4800 | 11.9975 | 400.08 |

## Filter Choices

| file | MAF points | IIR A | IIR B | FIR weights | FIR cutoff [Hz] | approx transition [Hz] | window |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| sigA.csv | 400 | 0.990 | 0.010 | 801 | 10.0 | 49.9 | hamming |
| sigB.csv | 200 | 0.980 | 0.020 | 301 | 8.0 | 43.9 | hamming |
| sigC.csv | 100 | 0.980 | 0.020 | 201 | 5.0 | 49.8 | hamming |
| sigD.csv | 50 | 0.900 | 0.100 | 101 | 5.0 | 15.8 | hamming |

## Run

```sh
python -m pip install -r requirements.txt
python HW9.py
```

The script creates the FFT, MAF, IIR, and FIR plots for all four CSV files.
