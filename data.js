window.BENCHMARK_DATA = {
  "lastUpdate": 1745862020401,
  "repoUrl": "https://github.com/getsentry/sentry-native",
  "entries": {
    "Linux": [
      {
        "commit": {
          "author": {
            "email": "jpnurmi@gmail.com",
            "name": "J-P Nurmi",
            "username": "jpnurmi"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "01b740a954bbb339f263773dbacabbbe99eb0c41",
          "message": "test: benchmark SDK startup (#1209)\n\n* test: benchmark SDK startup\n\n* CI: permissions required for github-action-benchmark\n\nhttps://github.com/benchmark-action/github-action-benchmark?tab=readme-ov-file#charts-on-github-pages-1\n\n* tests/conftest.py: extract constants\n\n* Warmup run\n\n* Add min/max/mean/stddev\n\n* Filter out empty string from formatting\n\n* Move tests/unit/benchmark* -> tests/benchmark/\n\n* Uncomment github-action-benchmark",
          "timestamp": "2025-04-28T19:38:01+02:00",
          "tree_id": "59abb8c8222185e7084fa7d5fc8d6a18ffa2072b",
          "url": "https://github.com/getsentry/sentry-native/commit/01b740a954bbb339f263773dbacabbbe99eb0c41"
        },
        "date": 1745862020082,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 0.8095489999959682,
            "unit": "ms",
            "extra": "Min 0.769ms\nMax 0.846ms\nMean 0.806ms\nStdDev 0.035ms\nMedian 0.810ms\nCPU 0.805ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 0.8225640000034673,
            "unit": "ms",
            "extra": "Min 0.777ms\nMax 0.870ms\nMean 0.820ms\nStdDev 0.036ms\nMedian 0.823ms\nCPU 0.819ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 3.5213050000209023,
            "unit": "ms",
            "extra": "Min 3.399ms\nMax 3.566ms\nMean 3.494ms\nStdDev 0.071ms\nMedian 3.521ms\nCPU 1.764ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.014426999996430823,
            "unit": "ms",
            "extra": "Min 0.014ms\nMax 0.035ms\nMean 0.018ms\nStdDev 0.009ms\nMedian 0.014ms\nCPU 0.017ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.026350000013053432,
            "unit": "ms",
            "extra": "Min 0.025ms\nMax 0.027ms\nMean 0.026ms\nStdDev 0.001ms\nMedian 0.026ms\nCPU 0.025ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 2.1143399999914436,
            "unit": "ms",
            "extra": "Min 2.091ms\nMax 2.352ms\nMean 2.161ms\nStdDev 0.108ms\nMedian 2.114ms\nCPU 0.656ms"
          }
        ]
      }
    ]
  }
}