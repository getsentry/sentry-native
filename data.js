window.BENCHMARK_DATA = {
  "lastUpdate": 1745927849571,
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
      },
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
          "id": "76f82d7de2fc52c7fe6a49910a5b0a71ae12977e",
          "message": "docs: add benchmark sections to README.md & CONTRIBUTING.md (#1217)",
          "timestamp": "2025-04-29T12:45:54+02:00",
          "tree_id": "5b80ad6b379f1bb6fe22dc7062f4356a11614023",
          "url": "https://github.com/getsentry/sentry-native/commit/76f82d7de2fc52c7fe6a49910a5b0a71ae12977e"
        },
        "date": 1745923693862,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 0.7104510000033315,
            "unit": "ms",
            "extra": "Min 0.685ms\nMax 0.737ms\nMean 0.711ms\nStdDev 0.020ms\nMedian 0.710ms\nCPU 0.710ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 0.7114809999961835,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.720ms\nMean 0.709ms\nStdDev 0.012ms\nMedian 0.711ms\nCPU 0.708ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 2.915047000016102,
            "unit": "ms",
            "extra": "Min 2.824ms\nMax 3.115ms\nMean 2.927ms\nStdDev 0.117ms\nMedian 2.915ms\nCPU 1.512ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.012213000019301035,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.022201000007271432,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.048ms\nMean 0.027ms\nStdDev 0.012ms\nMedian 0.022ms\nCPU 0.026ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 1.7800770000064858,
            "unit": "ms",
            "extra": "Min 1.757ms\nMax 1.874ms\nMean 1.793ms\nStdDev 0.046ms\nMedian 1.780ms\nCPU 0.547ms"
          }
        ]
      },
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
          "id": "f365474360fbe0036910ecd9d2589993544c2507",
          "message": "test: use more descriptive benchmark chart labels (#1218)",
          "timestamp": "2025-04-29T13:53:23+02:00",
          "tree_id": "b91e3a201a1870ae389c8ff1c1eeeef2a1c9fa04",
          "url": "https://github.com/getsentry/sentry-native/commit/f365474360fbe0036910ecd9d2589993544c2507"
        },
        "date": 1745927849235,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.6944560000192723,
            "unit": "ms",
            "extra": "Min 0.676ms\nMax 0.699ms\nMean 0.688ms\nStdDev 0.011ms\nMedian 0.694ms\nCPU 0.688ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7239209999738705,
            "unit": "ms",
            "extra": "Min 0.697ms\nMax 0.759ms\nMean 0.723ms\nStdDev 0.024ms\nMedian 0.724ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9084330000159753,
            "unit": "ms",
            "extra": "Min 2.859ms\nMax 3.004ms\nMean 2.929ms\nStdDev 0.064ms\nMedian 2.908ms\nCPU 1.518ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01282399995261585,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02277300001196636,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 2.0959310000421283,
            "unit": "ms",
            "extra": "Min 1.937ms\nMax 2.457ms\nMean 2.192ms\nStdDev 0.225ms\nMedian 2.096ms\nCPU 0.666ms"
          }
        ]
      }
    ],
    "macOS": [
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
        "date": 1745862044514,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 2.7705410000180564,
            "unit": "ms",
            "extra": "Min 2.502ms\nMax 3.283ms\nMean 2.865ms\nStdDev 0.304ms\nMedian 2.771ms\nCPU 1.643ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 3.2464999999888278,
            "unit": "ms",
            "extra": "Min 3.097ms\nMax 4.351ms\nMean 3.508ms\nStdDev 0.536ms\nMedian 3.246ms\nCPU 2.050ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 11.47666700001082,
            "unit": "ms",
            "extra": "Min 10.707ms\nMax 23.460ms\nMean 14.110ms\nStdDev 5.391ms\nMedian 11.477ms\nCPU 5.286ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.03070799999704832,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.043ms\nMean 0.031ms\nStdDev 0.013ms\nMedian 0.031ms\nCPU 0.030ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.17279200005759776,
            "unit": "ms",
            "extra": "Min 0.149ms\nMax 0.225ms\nMean 0.175ms\nStdDev 0.031ms\nMedian 0.173ms\nCPU 0.175ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 7.032834000028743,
            "unit": "ms",
            "extra": "Min 6.570ms\nMax 10.323ms\nMean 7.985ms\nStdDev 1.655ms\nMedian 7.033ms\nCPU 1.068ms"
          }
        ]
      },
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
          "id": "76f82d7de2fc52c7fe6a49910a5b0a71ae12977e",
          "message": "docs: add benchmark sections to README.md & CONTRIBUTING.md (#1217)",
          "timestamp": "2025-04-29T12:45:54+02:00",
          "tree_id": "5b80ad6b379f1bb6fe22dc7062f4356a11614023",
          "url": "https://github.com/getsentry/sentry-native/commit/76f82d7de2fc52c7fe6a49910a5b0a71ae12977e"
        },
        "date": 1745923697544,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 3.0543340000122043,
            "unit": "ms",
            "extra": "Min 2.733ms\nMax 3.161ms\nMean 3.012ms\nStdDev 0.162ms\nMedian 3.054ms\nCPU 1.611ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 2.900167000007059,
            "unit": "ms",
            "extra": "Min 2.818ms\nMax 3.200ms\nMean 2.976ms\nStdDev 0.162ms\nMedian 2.900ms\nCPU 1.721ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 9.695250000049782,
            "unit": "ms",
            "extra": "Min 8.815ms\nMax 10.285ms\nMean 9.604ms\nStdDev 0.585ms\nMedian 9.695ms\nCPU 3.491ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.02612499997667328,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.034ms\nMean 0.024ms\nStdDev 0.008ms\nMedian 0.026ms\nCPU 0.023ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.17837499990491779,
            "unit": "ms",
            "extra": "Min 0.137ms\nMax 0.211ms\nMean 0.178ms\nStdDev 0.027ms\nMedian 0.178ms\nCPU 0.178ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 4.895125000075495,
            "unit": "ms",
            "extra": "Min 4.426ms\nMax 6.221ms\nMean 5.161ms\nStdDev 0.692ms\nMedian 4.895ms\nCPU 0.755ms"
          }
        ]
      },
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
          "id": "f365474360fbe0036910ecd9d2589993544c2507",
          "message": "test: use more descriptive benchmark chart labels (#1218)",
          "timestamp": "2025-04-29T13:53:23+02:00",
          "tree_id": "b91e3a201a1870ae389c8ff1c1eeeef2a1c9fa04",
          "url": "https://github.com/getsentry/sentry-native/commit/f365474360fbe0036910ecd9d2589993544c2507"
        },
        "date": 1745927759765,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.170166000018071,
            "unit": "ms",
            "extra": "Min 2.782ms\nMax 3.324ms\nMean 3.129ms\nStdDev 0.210ms\nMedian 3.170ms\nCPU 1.735ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.2859169999710502,
            "unit": "ms",
            "extra": "Min 2.731ms\nMax 3.718ms\nMean 3.257ms\nStdDev 0.457ms\nMedian 3.286ms\nCPU 1.860ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.880166999982066,
            "unit": "ms",
            "extra": "Min 9.727ms\nMax 12.806ms\nMean 10.843ms\nStdDev 1.224ms\nMedian 10.880ms\nCPU 4.022ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.036833999956797925,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.041ms\nMean 0.032ms\nStdDev 0.013ms\nMedian 0.037ms\nCPU 0.031ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2702919999819642,
            "unit": "ms",
            "extra": "Min 0.229ms\nMax 0.321ms\nMean 0.268ms\nStdDev 0.037ms\nMedian 0.270ms\nCPU 0.267ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.45370799995726,
            "unit": "ms",
            "extra": "Min 6.277ms\nMax 13.833ms\nMean 8.364ms\nStdDev 3.116ms\nMedian 7.454ms\nCPU 1.203ms"
          }
        ]
      }
    ],
    "Windows": [
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
        "date": 1745862177550,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 7.0536999999148975,
            "unit": "ms",
            "extra": "Min 6.748ms\nMax 8.967ms\nMean 7.484ms\nStdDev 0.938ms\nMedian 7.054ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 7.239299999923787,
            "unit": "ms",
            "extra": "Min 7.132ms\nMax 7.419ms\nMean 7.276ms\nStdDev 0.124ms\nMedian 7.239ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 16.937500000040018,
            "unit": "ms",
            "extra": "Min 16.876ms\nMax 17.940ms\nMean 17.249ms\nStdDev 0.476ms\nMedian 16.938ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.31930000000102154,
            "unit": "ms",
            "extra": "Min 0.314ms\nMax 0.338ms\nMean 0.322ms\nStdDev 0.010ms\nMedian 0.319ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 9.357399999998961,
            "unit": "ms",
            "extra": "Min 9.008ms\nMax 9.469ms\nMean 9.309ms\nStdDev 0.178ms\nMedian 9.357ms"
          }
        ]
      },
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
          "id": "76f82d7de2fc52c7fe6a49910a5b0a71ae12977e",
          "message": "docs: add benchmark sections to README.md & CONTRIBUTING.md (#1217)",
          "timestamp": "2025-04-29T12:45:54+02:00",
          "tree_id": "5b80ad6b379f1bb6fe22dc7062f4356a11614023",
          "url": "https://github.com/getsentry/sentry-native/commit/76f82d7de2fc52c7fe6a49910a5b0a71ae12977e"
        },
        "date": 1745923862104,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "tests/benchmark.py::test_benchmark[init-inproc]",
            "value": 6.937600000014754,
            "unit": "ms",
            "extra": "Min 6.786ms\nMax 7.108ms\nMean 6.941ms\nStdDev 0.124ms\nMedian 6.938ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-breakpad]",
            "value": 7.598799999982475,
            "unit": "ms",
            "extra": "Min 7.271ms\nMax 11.541ms\nMean 8.430ms\nStdDev 1.774ms\nMedian 7.599ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[init-crashpad]",
            "value": 16.961799999990035,
            "unit": "ms",
            "extra": "Min 16.782ms\nMax 17.435ms\nMean 17.066ms\nStdDev 0.295ms\nMedian 16.962ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-inproc]",
            "value": 0.01190000000406144,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.023ms\nMean 0.013ms\nStdDev 0.006ms\nMedian 0.012ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-breakpad]",
            "value": 0.290400000039881,
            "unit": "ms",
            "extra": "Min 0.285ms\nMax 0.308ms\nMean 0.293ms\nStdDev 0.009ms\nMedian 0.290ms"
          },
          {
            "name": "tests/benchmark.py::test_benchmark[backend-crashpad]",
            "value": 8.9516000000458,
            "unit": "ms",
            "extra": "Min 8.791ms\nMax 9.202ms\nMean 8.968ms\nStdDev 0.180ms\nMedian 8.952ms"
          }
        ]
      }
    ]
  }
}