window.BENCHMARK_DATA = {
  "lastUpdate": 1747311269124,
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
            "name": "SDK init (inproc)",
            "value": 0.8095489999959682,
            "unit": "ms",
            "extra": "Min 0.769ms\nMax 0.846ms\nMean 0.806ms\nStdDev 0.035ms\nMedian 0.810ms\nCPU 0.805ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.8225640000034673,
            "unit": "ms",
            "extra": "Min 0.777ms\nMax 0.870ms\nMean 0.820ms\nStdDev 0.036ms\nMedian 0.823ms\nCPU 0.819ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.5213050000209023,
            "unit": "ms",
            "extra": "Min 3.399ms\nMax 3.566ms\nMean 3.494ms\nStdDev 0.071ms\nMedian 3.521ms\nCPU 1.764ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.014426999996430823,
            "unit": "ms",
            "extra": "Min 0.014ms\nMax 0.035ms\nMean 0.018ms\nStdDev 0.009ms\nMedian 0.014ms\nCPU 0.017ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.026350000013053432,
            "unit": "ms",
            "extra": "Min 0.025ms\nMax 0.027ms\nMean 0.026ms\nStdDev 0.001ms\nMedian 0.026ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (crashpad)",
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
            "name": "SDK init (inproc)",
            "value": 0.7104510000033315,
            "unit": "ms",
            "extra": "Min 0.685ms\nMax 0.737ms\nMean 0.711ms\nStdDev 0.020ms\nMedian 0.710ms\nCPU 0.710ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7114809999961835,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.720ms\nMean 0.709ms\nStdDev 0.012ms\nMedian 0.711ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.915047000016102,
            "unit": "ms",
            "extra": "Min 2.824ms\nMax 3.115ms\nMean 2.927ms\nStdDev 0.117ms\nMedian 2.915ms\nCPU 1.512ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012213000019301035,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022201000007271432,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.048ms\nMean 0.027ms\nStdDev 0.012ms\nMedian 0.022ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (crashpad)",
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
      },
      {
        "commit": {
          "author": {
            "email": "tustanivsky@gmail.com",
            "name": "Ivan Tustanivskyi",
            "username": "tustanivsky"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d97e52356f85952511496ce1913f8807bdf045b8",
          "message": "Add platform guard to fix Xbox build issues (#1220)\n\n* Exclude windows-specific code breaking the build for Xbox\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-04-30T12:12:18+03:00",
          "tree_id": "2f4f74d14a7d1826eeb79eb23438c66ccb5355be",
          "url": "https://github.com/getsentry/sentry-native/commit/d97e52356f85952511496ce1913f8807bdf045b8"
        },
        "date": 1746007382374,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.724224000009599,
            "unit": "ms",
            "extra": "Min 0.700ms\nMax 0.787ms\nMean 0.732ms\nStdDev 0.034ms\nMedian 0.724ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7080660000156058,
            "unit": "ms",
            "extra": "Min 0.701ms\nMax 0.747ms\nMean 0.718ms\nStdDev 0.020ms\nMedian 0.708ms\nCPU 0.717ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.978729000005842,
            "unit": "ms",
            "extra": "Min 2.927ms\nMax 3.293ms\nMean 3.058ms\nStdDev 0.156ms\nMedian 2.979ms\nCPU 1.516ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012484000023960107,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022661999992124038,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.029ms\nMean 0.024ms\nStdDev 0.003ms\nMedian 0.023ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7884729999764204,
            "unit": "ms",
            "extra": "Min 1.768ms\nMax 1.839ms\nMean 1.797ms\nStdDev 0.027ms\nMedian 1.788ms\nCPU 0.569ms"
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
          "id": "50d3e9146eb3897865fc6f02fe4a8390f890b8cc",
          "message": "test: assert crashpad attachment upload (#1222)",
          "timestamp": "2025-05-05T16:12:02+02:00",
          "tree_id": "d1d5b1e80a85af14327828e096e1f9deb468c3ca",
          "url": "https://github.com/getsentry/sentry-native/commit/50d3e9146eb3897865fc6f02fe4a8390f890b8cc"
        },
        "date": 1746454511757,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.6968209999911323,
            "unit": "ms",
            "extra": "Min 0.675ms\nMax 0.702ms\nMean 0.690ms\nStdDev 0.012ms\nMedian 0.697ms\nCPU 0.690ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6907509999791728,
            "unit": "ms",
            "extra": "Min 0.686ms\nMax 0.712ms\nMean 0.695ms\nStdDev 0.010ms\nMedian 0.691ms\nCPU 0.694ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8860819999749765,
            "unit": "ms",
            "extra": "Min 2.831ms\nMax 2.987ms\nMean 2.888ms\nStdDev 0.061ms\nMedian 2.886ms\nCPU 1.502ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01217300001599142,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02185099998541773,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8296549999945455,
            "unit": "ms",
            "extra": "Min 1.781ms\nMax 1.832ms\nMean 1.811ms\nStdDev 0.027ms\nMedian 1.830ms\nCPU 0.556ms"
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
          "id": "5dd6782d06a110095cd51d3f45df7e455c006ccf",
          "message": "chore: make update-test-discovery sort locale-independent (#1227)",
          "timestamp": "2025-05-07T08:59:35+02:00",
          "tree_id": "bd47ca4133e3657c4e31cdf5eb9059381bae35f8",
          "url": "https://github.com/getsentry/sentry-native/commit/5dd6782d06a110095cd51d3f45df7e455c006ccf"
        },
        "date": 1746601311790,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7003249999968375,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.777ms\nMean 0.715ms\nStdDev 0.035ms\nMedian 0.700ms\nCPU 0.702ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7210740000118676,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.759ms\nMean 0.728ms\nStdDev 0.023ms\nMedian 0.721ms\nCPU 0.727ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9094409999856907,
            "unit": "ms",
            "extra": "Min 2.860ms\nMax 2.961ms\nMean 2.915ms\nStdDev 0.043ms\nMedian 2.909ms\nCPU 1.548ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01232299996445363,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.028ms\nMean 0.016ms\nStdDev 0.007ms\nMedian 0.012ms\nCPU 0.015ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022381999997378443,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7681439999819304,
            "unit": "ms",
            "extra": "Min 1.751ms\nMax 1.800ms\nMean 1.770ms\nStdDev 0.019ms\nMedian 1.768ms\nCPU 0.558ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8755115b8c37cc98ebb79dd518195de84164c847",
          "message": "feat: add `sentry_value_new_user` (#1228)\n\n* add sentry_value_new_user\n\n* add test\n\n* add comment\n\n* add test scenario\n\n* CHANGELOG.md\n\n* update user.id to be string\n\n* cleanup + add new_user_n implementation\n\n* use string_n instead\n\n* changelog formatting",
          "timestamp": "2025-05-07T17:40:39+02:00",
          "tree_id": "0e486f1c2446d75eeec9b866bd35b1e358ead766",
          "url": "https://github.com/getsentry/sentry-native/commit/8755115b8c37cc98ebb79dd518195de84164c847"
        },
        "date": 1746632770840,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.691452999944886,
            "unit": "ms",
            "extra": "Min 0.677ms\nMax 0.717ms\nMean 0.691ms\nStdDev 0.016ms\nMedian 0.691ms\nCPU 0.691ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7061000000021522,
            "unit": "ms",
            "extra": "Min 0.683ms\nMax 0.742ms\nMean 0.714ms\nStdDev 0.025ms\nMedian 0.706ms\nCPU 0.714ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9253269999571785,
            "unit": "ms",
            "extra": "Min 2.906ms\nMax 3.108ms\nMean 2.970ms\nStdDev 0.084ms\nMedian 2.925ms\nCPU 1.539ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012633999972422316,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02228200003173697,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.812378000067838,
            "unit": "ms",
            "extra": "Min 1.725ms\nMax 1.815ms\nMean 1.780ms\nStdDev 0.047ms\nMedian 1.812ms\nCPU 0.540ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "6349682+vaind@users.noreply.github.com",
            "name": "Ivan Dlugos",
            "username": "vaind"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ba3d4167ff408f11245c986802ccf5ba6c073f31",
          "message": "chore: PS downstream SDK support (#1224)",
          "timestamp": "2025-05-13T07:38:49+02:00",
          "tree_id": "668de9b24f4bf4be9c3dc63a2d640d138116e24b",
          "url": "https://github.com/getsentry/sentry-native/commit/ba3d4167ff408f11245c986802ccf5ba6c073f31"
        },
        "date": 1747114861025,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7040920000065398,
            "unit": "ms",
            "extra": "Min 0.687ms\nMax 0.758ms\nMean 0.711ms\nStdDev 0.028ms\nMedian 0.704ms\nCPU 0.702ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6991629999788529,
            "unit": "ms",
            "extra": "Min 0.682ms\nMax 0.723ms\nMean 0.701ms\nStdDev 0.019ms\nMedian 0.699ms\nCPU 0.701ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9508380000038414,
            "unit": "ms",
            "extra": "Min 2.839ms\nMax 3.166ms\nMean 2.955ms\nStdDev 0.133ms\nMedian 2.951ms\nCPU 1.484ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012282999989565724,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022311999998692045,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7390229999989515,
            "unit": "ms",
            "extra": "Min 1.714ms\nMax 1.800ms\nMean 1.745ms\nStdDev 0.036ms\nMedian 1.739ms\nCPU 0.543ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bbca52a36c6811c5c18882099cb7e27fd10621d2",
          "message": "fix: trace sync improvements (#1200)\n\n* make set_trace write into propagation_context\n\n* check propagation context on transaction creation\n\n* clone instead of steal the propagation context trace data\n\n* apply propagation context when scoping transaction/span\n\n* progress\n\n* no longer apply trace data on span scoping\n\n* populate propagation_context with random trace_id and span_id\n\n* merge propagation context into contexts for event\n\n* add tests\n\n* only set propagation_context for TwP\n\n* finish all started spans\n\n* always init propagation_context\n\n* cleanup + CHANGELOG.md\n\n* extract init into static helper\n\n* first integration tests\n\n* more integration tests\n\n* remove todo\n\n* mark changes as breaking\n\n* CHANGELOG.md\n\n* Update CHANGELOG.md\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-13T11:26:04+02:00",
          "tree_id": "24f34cc1d128baf724703ef01f788002eac444d7",
          "url": "https://github.com/getsentry/sentry-native/commit/bbca52a36c6811c5c18882099cb7e27fd10621d2"
        },
        "date": 1747128495126,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7144439999819951,
            "unit": "ms",
            "extra": "Min 0.695ms\nMax 0.858ms\nMean 0.739ms\nStdDev 0.067ms\nMedian 0.714ms\nCPU 0.714ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7147750000058295,
            "unit": "ms",
            "extra": "Min 0.686ms\nMax 0.764ms\nMean 0.720ms\nStdDev 0.029ms\nMedian 0.715ms\nCPU 0.720ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8703240000140795,
            "unit": "ms",
            "extra": "Min 2.852ms\nMax 2.935ms\nMean 2.892ms\nStdDev 0.039ms\nMedian 2.870ms\nCPU 1.493ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012293000054341974,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.021730000014485995,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.048ms\nMean 0.027ms\nStdDev 0.012ms\nMedian 0.022ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7886040000121284,
            "unit": "ms",
            "extra": "Min 1.713ms\nMax 1.841ms\nMean 1.780ms\nStdDev 0.055ms\nMedian 1.789ms\nCPU 0.538ms"
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
          "id": "9a334b07e2e57472d92f4532c632339d0085402a",
          "message": "fix: support musl on Linux (#1233)\n\n* fix: libunwind as the macOS unwinder\n\n* fix: use libunwind with musl\n\n* ci: experiment with Alpine Linux (musl)\n\n* chore: comment out debug output\n\n* chore: update external/crashpad\n\n* fixup: don't set SENTRY_WITH_LIBUNWIND on APPLE\n\n* chore: find libunwind.h & libunwind.so\n\n* ci: libunwind vs. llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: update external/crashpad\n\n* chore: clean up\n\n* chore: update CHANGELOG.md\n\n* ci: gcc+libunwind vs. clang+llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: clean up extra newlines\n\n* build: pick libunwind.a when SENTRY_BUILD_SHARED_LIBS=0\n\n* build: add SENTRY_LIBUNWIND_SHARED option\n\n* build: lzma\n\n* drop llvm-libunwind\n\n* Update src/unwinder/sentry_unwinder_libunwind.c\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* chore: fix formatting\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-14T10:29:40+02:00",
          "tree_id": "f7ace77739aaf925a1fe60fadfb7e08f340feb1d",
          "url": "https://github.com/getsentry/sentry-native/commit/9a334b07e2e57472d92f4532c632339d0085402a"
        },
        "date": 1747211517809,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7078600000056667,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.748ms\nMean 0.710ms\nStdDev 0.023ms\nMedian 0.708ms\nCPU 0.698ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6525459999977556,
            "unit": "ms",
            "extra": "Min 0.650ms\nMax 0.723ms\nMean 0.672ms\nStdDev 0.032ms\nMedian 0.653ms\nCPU 0.672ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.7162330000010115,
            "unit": "ms",
            "extra": "Min 2.640ms\nMax 2.827ms\nMean 2.726ms\nStdDev 0.084ms\nMedian 2.716ms\nCPU 1.429ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012171999998145111,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.020408000011684635,
            "unit": "ms",
            "extra": "Min 0.019ms\nMax 0.022ms\nMean 0.021ms\nStdDev 0.001ms\nMedian 0.020ms\nCPU 0.020ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.6830680000055054,
            "unit": "ms",
            "extra": "Min 1.600ms\nMax 1.768ms\nMean 1.680ms\nStdDev 0.066ms\nMedian 1.683ms\nCPU 0.523ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "committer": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "distinct": true,
          "id": "5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a",
          "message": "Merge branch 'release/0.8.5'",
          "timestamp": "2025-05-14T13:10:30Z",
          "tree_id": "6d1f156d2a478609d441d118330310920630c4ec",
          "url": "https://github.com/getsentry/sentry-native/commit/5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a"
        },
        "date": 1747228367287,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7364020000011351,
            "unit": "ms",
            "extra": "Min 0.711ms\nMax 0.818ms\nMean 0.746ms\nStdDev 0.044ms\nMedian 0.736ms\nCPU 0.734ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7402299999910156,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.775ms\nMean 0.733ms\nStdDev 0.033ms\nMedian 0.740ms\nCPU 0.731ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.947684999980993,
            "unit": "ms",
            "extra": "Min 2.816ms\nMax 3.139ms\nMean 2.963ms\nStdDev 0.119ms\nMedian 2.948ms\nCPU 1.548ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01233300002922988,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022381999997378443,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7725040000300396,
            "unit": "ms",
            "extra": "Min 1.750ms\nMax 1.876ms\nMean 1.797ms\nStdDev 0.051ms\nMedian 1.773ms\nCPU 0.545ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "mischan@abovevacant.com",
            "name": "Mischan Toosarani-Hausberger",
            "username": "supervacuus"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2c2ef4479c2717cc2055c3939d5bbab675ab6a69",
          "message": "chore: bump codecov-action to 5.4.2 (#1242)",
          "timestamp": "2025-05-14T18:23:48+02:00",
          "tree_id": "24e5b4492ef5466d2b194fc634299667d390ba7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2c2ef4479c2717cc2055c3939d5bbab675ab6a69"
        },
        "date": 1747239966938,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7666710000080457,
            "unit": "ms",
            "extra": "Min 0.717ms\nMax 0.781ms\nMean 0.752ms\nStdDev 0.030ms\nMedian 0.767ms\nCPU 0.751ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7307019999984732,
            "unit": "ms",
            "extra": "Min 0.695ms\nMax 0.762ms\nMean 0.731ms\nStdDev 0.028ms\nMedian 0.731ms\nCPU 0.728ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8803220000099827,
            "unit": "ms",
            "extra": "Min 2.855ms\nMax 3.036ms\nMean 2.915ms\nStdDev 0.074ms\nMedian 2.880ms\nCPU 1.520ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012352999988252122,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022531999945840653,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.772110999979759,
            "unit": "ms",
            "extra": "Min 1.754ms\nMax 1.862ms\nMean 1.797ms\nStdDev 0.045ms\nMedian 1.772ms\nCPU 0.565ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5224a83ca41207085f0694caca460bb33852efb0",
          "message": "feat: Add `before_send_transaction` (#1236)\n\n* initial before_send_transaction implementation\n\n* CHANGELOG.md\n\n* add before_transaction_data\n\n* add tests\n\n* rename `closure` to `user_data`\n\n* update CHANGELOG.md after merging main",
          "timestamp": "2025-05-15T12:26:44+02:00",
          "tree_id": "edf2411e42a743cd55bf1a5399e9ec0959b7c8d1",
          "url": "https://github.com/getsentry/sentry-native/commit/5224a83ca41207085f0694caca460bb33852efb0"
        },
        "date": 1747304940906,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.6943040000066958,
            "unit": "ms",
            "extra": "Min 0.685ms\nMax 0.745ms\nMean 0.704ms\nStdDev 0.025ms\nMedian 0.694ms\nCPU 0.704ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7113759999981539,
            "unit": "ms",
            "extra": "Min 0.689ms\nMax 0.735ms\nMean 0.710ms\nStdDev 0.020ms\nMedian 0.711ms\nCPU 0.710ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.108330000031856,
            "unit": "ms",
            "extra": "Min 2.891ms\nMax 3.443ms\nMean 3.097ms\nStdDev 0.220ms\nMedian 3.108ms\nCPU 1.520ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012844000025324931,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02235099998415535,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9240809999701014,
            "unit": "ms",
            "extra": "Min 1.843ms\nMax 2.033ms\nMean 1.939ms\nStdDev 0.074ms\nMedian 1.924ms\nCPU 0.589ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "mischan@abovevacant.com",
            "name": "Mischan Toosarani-Hausberger",
            "username": "supervacuus"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "3d15dda2181af0a93cb8db4cdc23577db95fc74b",
          "message": "chore: remove `twxs.cmake` from the recommended VSCode extensions (#1246)\n\nsee: https://github.com/twxs/vs.language.cmake/issues/119\r\n\r\nCMake Tools also now recommends uninstalling `twxs.cmake` when you install it. This is an unnecessary dev-setup prologue.",
          "timestamp": "2025-05-15T14:12:16+02:00",
          "tree_id": "615ea1b73964e7e9c8eb844234d3fca2932ac690",
          "url": "https://github.com/getsentry/sentry-native/commit/3d15dda2181af0a93cb8db4cdc23577db95fc74b"
        },
        "date": 1747311268570,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7228010000233098,
            "unit": "ms",
            "extra": "Min 0.711ms\nMax 0.732ms\nMean 0.722ms\nStdDev 0.009ms\nMedian 0.723ms\nCPU 0.722ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7113190000040959,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.794ms\nMean 0.725ms\nStdDev 0.041ms\nMedian 0.711ms\nCPU 0.713ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.93559399995047,
            "unit": "ms",
            "extra": "Min 2.797ms\nMax 3.647ms\nMean 3.049ms\nStdDev 0.341ms\nMedian 2.936ms\nCPU 1.502ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012303000005431386,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02160000002504603,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7520080000394955,
            "unit": "ms",
            "extra": "Min 1.738ms\nMax 1.821ms\nMean 1.773ms\nStdDev 0.040ms\nMedian 1.752ms\nCPU 0.541ms"
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
            "name": "SDK init (inproc)",
            "value": 2.7705410000180564,
            "unit": "ms",
            "extra": "Min 2.502ms\nMax 3.283ms\nMean 2.865ms\nStdDev 0.304ms\nMedian 2.771ms\nCPU 1.643ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.2464999999888278,
            "unit": "ms",
            "extra": "Min 3.097ms\nMax 4.351ms\nMean 3.508ms\nStdDev 0.536ms\nMedian 3.246ms\nCPU 2.050ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.47666700001082,
            "unit": "ms",
            "extra": "Min 10.707ms\nMax 23.460ms\nMean 14.110ms\nStdDev 5.391ms\nMedian 11.477ms\nCPU 5.286ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03070799999704832,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.043ms\nMean 0.031ms\nStdDev 0.013ms\nMedian 0.031ms\nCPU 0.030ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.17279200005759776,
            "unit": "ms",
            "extra": "Min 0.149ms\nMax 0.225ms\nMean 0.175ms\nStdDev 0.031ms\nMedian 0.173ms\nCPU 0.175ms"
          },
          {
            "name": "Backend startup (crashpad)",
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
            "name": "SDK init (inproc)",
            "value": 3.0543340000122043,
            "unit": "ms",
            "extra": "Min 2.733ms\nMax 3.161ms\nMean 3.012ms\nStdDev 0.162ms\nMedian 3.054ms\nCPU 1.611ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.900167000007059,
            "unit": "ms",
            "extra": "Min 2.818ms\nMax 3.200ms\nMean 2.976ms\nStdDev 0.162ms\nMedian 2.900ms\nCPU 1.721ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 9.695250000049782,
            "unit": "ms",
            "extra": "Min 8.815ms\nMax 10.285ms\nMean 9.604ms\nStdDev 0.585ms\nMedian 9.695ms\nCPU 3.491ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02612499997667328,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.034ms\nMean 0.024ms\nStdDev 0.008ms\nMedian 0.026ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.17837499990491779,
            "unit": "ms",
            "extra": "Min 0.137ms\nMax 0.211ms\nMean 0.178ms\nStdDev 0.027ms\nMedian 0.178ms\nCPU 0.178ms"
          },
          {
            "name": "Backend startup (crashpad)",
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
      },
      {
        "commit": {
          "author": {
            "email": "tustanivsky@gmail.com",
            "name": "Ivan Tustanivskyi",
            "username": "tustanivsky"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d97e52356f85952511496ce1913f8807bdf045b8",
          "message": "Add platform guard to fix Xbox build issues (#1220)\n\n* Exclude windows-specific code breaking the build for Xbox\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-04-30T12:12:18+03:00",
          "tree_id": "2f4f74d14a7d1826eeb79eb23438c66ccb5355be",
          "url": "https://github.com/getsentry/sentry-native/commit/d97e52356f85952511496ce1913f8807bdf045b8"
        },
        "date": 1746007613481,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.010209000000486,
            "unit": "ms",
            "extra": "Min 3.095ms\nMax 4.707ms\nMean 4.008ms\nStdDev 0.706ms\nMedian 4.010ms\nCPU 2.159ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.93770799999993,
            "unit": "ms",
            "extra": "Min 4.437ms\nMax 5.710ms\nMean 4.971ms\nStdDev 0.541ms\nMedian 4.938ms\nCPU 2.942ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.127457999942635,
            "unit": "ms",
            "extra": "Min 9.894ms\nMax 10.255ms\nMean 10.095ms\nStdDev 0.138ms\nMedian 10.127ms\nCPU 3.733ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02375000008214556,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.037ms\nMean 0.021ms\nStdDev 0.013ms\nMedian 0.024ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.20570800006680656,
            "unit": "ms",
            "extra": "Min 0.169ms\nMax 0.300ms\nMean 0.226ms\nStdDev 0.052ms\nMedian 0.206ms\nCPU 0.226ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.097499999896172,
            "unit": "ms",
            "extra": "Min 5.550ms\nMax 6.486ms\nMean 6.050ms\nStdDev 0.447ms\nMedian 6.097ms\nCPU 0.846ms"
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
          "id": "50d3e9146eb3897865fc6f02fe4a8390f890b8cc",
          "message": "test: assert crashpad attachment upload (#1222)",
          "timestamp": "2025-05-05T16:12:02+02:00",
          "tree_id": "d1d5b1e80a85af14327828e096e1f9deb468c3ca",
          "url": "https://github.com/getsentry/sentry-native/commit/50d3e9146eb3897865fc6f02fe4a8390f890b8cc"
        },
        "date": 1746454491564,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.695790999984183,
            "unit": "ms",
            "extra": "Min 2.991ms\nMax 4.273ms\nMean 3.666ms\nStdDev 0.466ms\nMedian 3.696ms\nCPU 2.105ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.096457999959057,
            "unit": "ms",
            "extra": "Min 2.780ms\nMax 3.670ms\nMean 3.146ms\nStdDev 0.324ms\nMedian 3.096ms\nCPU 1.824ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.838041000014073,
            "unit": "ms",
            "extra": "Min 13.532ms\nMax 25.012ms\nMean 19.205ms\nStdDev 4.242ms\nMedian 19.838ms\nCPU 7.186ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.054750000003878085,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.167ms\nMean 0.074ms\nStdDev 0.061ms\nMedian 0.055ms\nCPU 0.073ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2829169999927217,
            "unit": "ms",
            "extra": "Min 0.246ms\nMax 0.342ms\nMean 0.284ms\nStdDev 0.037ms\nMedian 0.283ms\nCPU 0.281ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.3168750000149885,
            "unit": "ms",
            "extra": "Min 6.147ms\nMax 8.697ms\nMean 7.213ms\nStdDev 0.980ms\nMedian 7.317ms\nCPU 1.120ms"
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
          "id": "5dd6782d06a110095cd51d3f45df7e455c006ccf",
          "message": "chore: make update-test-discovery sort locale-independent (#1227)",
          "timestamp": "2025-05-07T08:59:35+02:00",
          "tree_id": "bd47ca4133e3657c4e31cdf5eb9059381bae35f8",
          "url": "https://github.com/getsentry/sentry-native/commit/5dd6782d06a110095cd51d3f45df7e455c006ccf"
        },
        "date": 1746601356982,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.078916000049503,
            "unit": "ms",
            "extra": "Min 2.607ms\nMax 3.217ms\nMean 3.007ms\nStdDev 0.233ms\nMedian 3.079ms\nCPU 1.647ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.9582089999848904,
            "unit": "ms",
            "extra": "Min 2.808ms\nMax 2.968ms\nMean 2.931ms\nStdDev 0.069ms\nMedian 2.958ms\nCPU 1.635ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 8.701957999960541,
            "unit": "ms",
            "extra": "Min 8.459ms\nMax 8.792ms\nMean 8.672ms\nStdDev 0.138ms\nMedian 8.702ms\nCPU 3.102ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.014458000009653915,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.017ms\nMean 0.013ms\nStdDev 0.004ms\nMedian 0.014ms\nCPU 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.16554100000121252,
            "unit": "ms",
            "extra": "Min 0.133ms\nMax 0.199ms\nMean 0.167ms\nStdDev 0.025ms\nMedian 0.166ms\nCPU 0.167ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 4.554708999989998,
            "unit": "ms",
            "extra": "Min 4.325ms\nMax 4.843ms\nMean 4.573ms\nStdDev 0.223ms\nMedian 4.555ms\nCPU 0.621ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8755115b8c37cc98ebb79dd518195de84164c847",
          "message": "feat: add `sentry_value_new_user` (#1228)\n\n* add sentry_value_new_user\n\n* add test\n\n* add comment\n\n* add test scenario\n\n* CHANGELOG.md\n\n* update user.id to be string\n\n* cleanup + add new_user_n implementation\n\n* use string_n instead\n\n* changelog formatting",
          "timestamp": "2025-05-07T17:40:39+02:00",
          "tree_id": "0e486f1c2446d75eeec9b866bd35b1e358ead766",
          "url": "https://github.com/getsentry/sentry-native/commit/8755115b8c37cc98ebb79dd518195de84164c847"
        },
        "date": 1746632622074,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.7227500000085456,
            "unit": "ms",
            "extra": "Min 2.961ms\nMax 7.599ms\nMean 4.367ms\nStdDev 1.877ms\nMedian 3.723ms\nCPU 2.274ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.805832999977611,
            "unit": "ms",
            "extra": "Min 3.534ms\nMax 6.689ms\nMean 5.210ms\nStdDev 1.333ms\nMedian 4.806ms\nCPU 2.788ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.065709000002698,
            "unit": "ms",
            "extra": "Min 9.689ms\nMax 20.553ms\nMean 13.433ms\nStdDev 4.981ms\nMedian 10.066ms\nCPU 5.628ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02854199999546836,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.050ms\nMean 0.025ms\nStdDev 0.018ms\nMedian 0.029ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2101249999668653,
            "unit": "ms",
            "extra": "Min 0.203ms\nMax 0.282ms\nMean 0.223ms\nStdDev 0.033ms\nMedian 0.210ms\nCPU 0.223ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.761333000009472,
            "unit": "ms",
            "extra": "Min 5.336ms\nMax 6.608ms\nMean 5.887ms\nStdDev 0.478ms\nMedian 5.761ms\nCPU 0.796ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "6349682+vaind@users.noreply.github.com",
            "name": "Ivan Dlugos",
            "username": "vaind"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ba3d4167ff408f11245c986802ccf5ba6c073f31",
          "message": "chore: PS downstream SDK support (#1224)",
          "timestamp": "2025-05-13T07:38:49+02:00",
          "tree_id": "668de9b24f4bf4be9c3dc63a2d640d138116e24b",
          "url": "https://github.com/getsentry/sentry-native/commit/ba3d4167ff408f11245c986802ccf5ba6c073f31"
        },
        "date": 1747114924507,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.615833999991992,
            "unit": "ms",
            "extra": "Min 3.440ms\nMax 8.096ms\nMean 5.040ms\nStdDev 1.803ms\nMedian 4.616ms\nCPU 2.507ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.863917000079709,
            "unit": "ms",
            "extra": "Min 3.255ms\nMax 6.826ms\nMean 4.696ms\nStdDev 1.431ms\nMedian 4.864ms\nCPU 2.418ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.05050000000574,
            "unit": "ms",
            "extra": "Min 12.526ms\nMax 21.227ms\nMean 16.445ms\nStdDev 3.175ms\nMedian 16.051ms\nCPU 5.828ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.04612500003986497,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.064ms\nMean 0.038ms\nStdDev 0.027ms\nMedian 0.046ms\nCPU 0.038ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2662910000026386,
            "unit": "ms",
            "extra": "Min 0.226ms\nMax 0.286ms\nMean 0.257ms\nStdDev 0.024ms\nMedian 0.266ms\nCPU 0.257ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.20287499996175,
            "unit": "ms",
            "extra": "Min 5.554ms\nMax 8.189ms\nMean 6.932ms\nStdDev 1.100ms\nMedian 7.203ms\nCPU 0.948ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bbca52a36c6811c5c18882099cb7e27fd10621d2",
          "message": "fix: trace sync improvements (#1200)\n\n* make set_trace write into propagation_context\n\n* check propagation context on transaction creation\n\n* clone instead of steal the propagation context trace data\n\n* apply propagation context when scoping transaction/span\n\n* progress\n\n* no longer apply trace data on span scoping\n\n* populate propagation_context with random trace_id and span_id\n\n* merge propagation context into contexts for event\n\n* add tests\n\n* only set propagation_context for TwP\n\n* finish all started spans\n\n* always init propagation_context\n\n* cleanup + CHANGELOG.md\n\n* extract init into static helper\n\n* first integration tests\n\n* more integration tests\n\n* remove todo\n\n* mark changes as breaking\n\n* CHANGELOG.md\n\n* Update CHANGELOG.md\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-13T11:26:04+02:00",
          "tree_id": "24f34cc1d128baf724703ef01f788002eac444d7",
          "url": "https://github.com/getsentry/sentry-native/commit/bbca52a36c6811c5c18882099cb7e27fd10621d2"
        },
        "date": 1747128535666,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.580084000006536,
            "unit": "ms",
            "extra": "Min 4.085ms\nMax 10.155ms\nMean 6.770ms\nStdDev 2.205ms\nMedian 6.580ms\nCPU 3.778ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1887500000493674,
            "unit": "ms",
            "extra": "Min 3.075ms\nMax 3.799ms\nMean 3.322ms\nStdDev 0.286ms\nMedian 3.189ms\nCPU 1.773ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.81033400009801,
            "unit": "ms",
            "extra": "Min 10.290ms\nMax 22.919ms\nMean 13.160ms\nStdDev 5.472ms\nMedian 10.810ms\nCPU 4.020ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.033708000046317466,
            "unit": "ms",
            "extra": "Min 0.032ms\nMax 0.048ms\nMean 0.037ms\nStdDev 0.007ms\nMedian 0.034ms\nCPU 0.037ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.17174999993585516,
            "unit": "ms",
            "extra": "Min 0.160ms\nMax 0.248ms\nMean 0.190ms\nStdDev 0.035ms\nMedian 0.172ms\nCPU 0.190ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.707540999992489,
            "unit": "ms",
            "extra": "Min 5.521ms\nMax 19.114ms\nMean 8.334ms\nStdDev 6.027ms\nMedian 5.708ms\nCPU 1.121ms"
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
          "id": "9a334b07e2e57472d92f4532c632339d0085402a",
          "message": "fix: support musl on Linux (#1233)\n\n* fix: libunwind as the macOS unwinder\n\n* fix: use libunwind with musl\n\n* ci: experiment with Alpine Linux (musl)\n\n* chore: comment out debug output\n\n* chore: update external/crashpad\n\n* fixup: don't set SENTRY_WITH_LIBUNWIND on APPLE\n\n* chore: find libunwind.h & libunwind.so\n\n* ci: libunwind vs. llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: update external/crashpad\n\n* chore: clean up\n\n* chore: update CHANGELOG.md\n\n* ci: gcc+libunwind vs. clang+llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: clean up extra newlines\n\n* build: pick libunwind.a when SENTRY_BUILD_SHARED_LIBS=0\n\n* build: add SENTRY_LIBUNWIND_SHARED option\n\n* build: lzma\n\n* drop llvm-libunwind\n\n* Update src/unwinder/sentry_unwinder_libunwind.c\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* chore: fix formatting\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-14T10:29:40+02:00",
          "tree_id": "f7ace77739aaf925a1fe60fadfb7e08f340feb1d",
          "url": "https://github.com/getsentry/sentry-native/commit/9a334b07e2e57472d92f4532c632339d0085402a"
        },
        "date": 1747211537593,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.711707999992541,
            "unit": "ms",
            "extra": "Min 2.626ms\nMax 2.930ms\nMean 2.739ms\nStdDev 0.114ms\nMedian 2.712ms\nCPU 1.461ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1272500000341097,
            "unit": "ms",
            "extra": "Min 2.933ms\nMax 3.388ms\nMean 3.127ms\nStdDev 0.178ms\nMedian 3.127ms\nCPU 1.786ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.037917000033758,
            "unit": "ms",
            "extra": "Min 9.549ms\nMax 14.149ms\nMean 10.764ms\nStdDev 1.929ms\nMedian 10.038ms\nCPU 3.847ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.029208999990260054,
            "unit": "ms",
            "extra": "Min 0.025ms\nMax 0.123ms\nMean 0.047ms\nStdDev 0.042ms\nMedian 0.029ms\nCPU 0.047ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30512500001123044,
            "unit": "ms",
            "extra": "Min 0.204ms\nMax 0.348ms\nMean 0.283ms\nStdDev 0.059ms\nMedian 0.305ms\nCPU 0.282ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.968957999982649,
            "unit": "ms",
            "extra": "Min 5.311ms\nMax 7.084ms\nMean 6.034ms\nStdDev 0.649ms\nMedian 5.969ms\nCPU 0.887ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "committer": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "distinct": true,
          "id": "5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a",
          "message": "Merge branch 'release/0.8.5'",
          "timestamp": "2025-05-14T13:10:30Z",
          "tree_id": "6d1f156d2a478609d441d118330310920630c4ec",
          "url": "https://github.com/getsentry/sentry-native/commit/5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a"
        },
        "date": 1747228382222,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.1988749999811716,
            "unit": "ms",
            "extra": "Min 3.044ms\nMax 3.609ms\nMean 3.281ms\nStdDev 0.221ms\nMedian 3.199ms\nCPU 1.816ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1663329999958023,
            "unit": "ms",
            "extra": "Min 2.991ms\nMax 3.318ms\nMean 3.163ms\nStdDev 0.149ms\nMedian 3.166ms\nCPU 1.739ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.948959000018021,
            "unit": "ms",
            "extra": "Min 10.260ms\nMax 13.920ms\nMean 11.296ms\nStdDev 1.510ms\nMedian 10.949ms\nCPU 4.222ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009207999994487182,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.467ms\nMean 0.104ms\nStdDev 0.203ms\nMedian 0.009ms\nCPU 0.104ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.16916700002411744,
            "unit": "ms",
            "extra": "Min 0.149ms\nMax 0.201ms\nMean 0.173ms\nStdDev 0.023ms\nMedian 0.169ms\nCPU 0.173ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.571707999990849,
            "unit": "ms",
            "extra": "Min 5.306ms\nMax 11.884ms\nMean 7.146ms\nStdDev 2.779ms\nMedian 5.572ms\nCPU 0.932ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "mischan@abovevacant.com",
            "name": "Mischan Toosarani-Hausberger",
            "username": "supervacuus"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2c2ef4479c2717cc2055c3939d5bbab675ab6a69",
          "message": "chore: bump codecov-action to 5.4.2 (#1242)",
          "timestamp": "2025-05-14T18:23:48+02:00",
          "tree_id": "24e5b4492ef5466d2b194fc634299667d390ba7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2c2ef4479c2717cc2055c3939d5bbab675ab6a69"
        },
        "date": 1747240005050,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.970833000010998,
            "unit": "ms",
            "extra": "Min 3.826ms\nMax 4.266ms\nMean 4.019ms\nStdDev 0.171ms\nMedian 3.971ms\nCPU 2.153ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.535917000026757,
            "unit": "ms",
            "extra": "Min 4.064ms\nMax 11.590ms\nMean 6.160ms\nStdDev 3.163ms\nMedian 4.536ms\nCPU 3.149ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 13.380999999981213,
            "unit": "ms",
            "extra": "Min 10.392ms\nMax 16.699ms\nMean 13.160ms\nStdDev 2.445ms\nMedian 13.381ms\nCPU 4.863ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.037458999997852516,
            "unit": "ms",
            "extra": "Min 0.015ms\nMax 0.091ms\nMean 0.044ms\nStdDev 0.028ms\nMedian 0.037ms\nCPU 0.044ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.4047080000191272,
            "unit": "ms",
            "extra": "Min 0.260ms\nMax 1.272ms\nMean 0.540ms\nStdDev 0.415ms\nMedian 0.405ms\nCPU 0.523ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.476500000005217,
            "unit": "ms",
            "extra": "Min 7.042ms\nMax 13.414ms\nMean 9.810ms\nStdDev 2.328ms\nMedian 9.477ms\nCPU 1.373ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5224a83ca41207085f0694caca460bb33852efb0",
          "message": "feat: Add `before_send_transaction` (#1236)\n\n* initial before_send_transaction implementation\n\n* CHANGELOG.md\n\n* add before_transaction_data\n\n* add tests\n\n* rename `closure` to `user_data`\n\n* update CHANGELOG.md after merging main",
          "timestamp": "2025-05-15T12:26:44+02:00",
          "tree_id": "edf2411e42a743cd55bf1a5399e9ec0959b7c8d1",
          "url": "https://github.com/getsentry/sentry-native/commit/5224a83ca41207085f0694caca460bb33852efb0"
        },
        "date": 1747305359320,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.0210410000108823,
            "unit": "ms",
            "extra": "Min 2.818ms\nMax 3.249ms\nMean 3.045ms\nStdDev 0.186ms\nMedian 3.021ms\nCPU 1.654ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.8340419999940423,
            "unit": "ms",
            "extra": "Min 3.030ms\nMax 7.752ms\nMean 4.377ms\nStdDev 1.917ms\nMedian 3.834ms\nCPU 2.381ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.823916999996527,
            "unit": "ms",
            "extra": "Min 13.666ms\nMax 21.529ms\nMean 18.529ms\nStdDev 3.439ms\nMedian 19.824ms\nCPU 6.812ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.040957999999591266,
            "unit": "ms",
            "extra": "Min 0.036ms\nMax 0.060ms\nMean 0.046ms\nStdDev 0.010ms\nMedian 0.041ms\nCPU 0.045ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2680830000372225,
            "unit": "ms",
            "extra": "Min 0.217ms\nMax 0.374ms\nMean 0.287ms\nStdDev 0.072ms\nMedian 0.268ms\nCPU 0.284ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.912083999997321,
            "unit": "ms",
            "extra": "Min 6.067ms\nMax 15.925ms\nMean 10.390ms\nStdDev 4.885ms\nMedian 7.912ms\nCPU 1.483ms"
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
            "name": "SDK init (inproc)",
            "value": 7.0536999999148975,
            "unit": "ms",
            "extra": "Min 6.748ms\nMax 8.967ms\nMean 7.484ms\nStdDev 0.938ms\nMedian 7.054ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.239299999923787,
            "unit": "ms",
            "extra": "Min 7.132ms\nMax 7.419ms\nMean 7.276ms\nStdDev 0.124ms\nMedian 7.239ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.937500000040018,
            "unit": "ms",
            "extra": "Min 16.876ms\nMax 17.940ms\nMean 17.249ms\nStdDev 0.476ms\nMedian 16.938ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31930000000102154,
            "unit": "ms",
            "extra": "Min 0.314ms\nMax 0.338ms\nMean 0.322ms\nStdDev 0.010ms\nMedian 0.319ms"
          },
          {
            "name": "Backend startup (crashpad)",
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
            "name": "SDK init (inproc)",
            "value": 6.937600000014754,
            "unit": "ms",
            "extra": "Min 6.786ms\nMax 7.108ms\nMean 6.941ms\nStdDev 0.124ms\nMedian 6.938ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.598799999982475,
            "unit": "ms",
            "extra": "Min 7.271ms\nMax 11.541ms\nMean 8.430ms\nStdDev 1.774ms\nMedian 7.599ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.961799999990035,
            "unit": "ms",
            "extra": "Min 16.782ms\nMax 17.435ms\nMean 17.066ms\nStdDev 0.295ms\nMedian 16.962ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01190000000406144,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.023ms\nMean 0.013ms\nStdDev 0.006ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.290400000039881,
            "unit": "ms",
            "extra": "Min 0.285ms\nMax 0.308ms\nMean 0.293ms\nStdDev 0.009ms\nMedian 0.290ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.9516000000458,
            "unit": "ms",
            "extra": "Min 8.791ms\nMax 9.202ms\nMean 8.968ms\nStdDev 0.180ms\nMedian 8.952ms"
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
        "date": 1745927904299,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.062200000007124,
            "unit": "ms",
            "extra": "Min 6.925ms\nMax 7.541ms\nMean 7.109ms\nStdDev 0.251ms\nMedian 7.062ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.339999999999236,
            "unit": "ms",
            "extra": "Min 7.253ms\nMax 7.655ms\nMean 7.419ms\nStdDev 0.162ms\nMedian 7.340ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.09040000002915,
            "unit": "ms",
            "extra": "Min 16.978ms\nMax 18.970ms\nMean 17.542ms\nStdDev 0.839ms\nMedian 17.090ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010900000006586197,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.014ms\nMean 0.011ms\nStdDev 0.002ms\nMedian 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3052000000707267,
            "unit": "ms",
            "extra": "Min 0.298ms\nMax 0.364ms\nMean 0.321ms\nStdDev 0.027ms\nMedian 0.305ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.221400000001267,
            "unit": "ms",
            "extra": "Min 9.197ms\nMax 9.282ms\nMean 9.232ms\nStdDev 0.036ms\nMedian 9.221ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "tustanivsky@gmail.com",
            "name": "Ivan Tustanivskyi",
            "username": "tustanivsky"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "d97e52356f85952511496ce1913f8807bdf045b8",
          "message": "Add platform guard to fix Xbox build issues (#1220)\n\n* Exclude windows-specific code breaking the build for Xbox\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* Update sentry_screenshot_windows.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-04-30T12:12:18+03:00",
          "tree_id": "2f4f74d14a7d1826eeb79eb23438c66ccb5355be",
          "url": "https://github.com/getsentry/sentry-native/commit/d97e52356f85952511496ce1913f8807bdf045b8"
        },
        "date": 1746007927708,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.885600000032355,
            "unit": "ms",
            "extra": "Min 6.828ms\nMax 7.230ms\nMean 6.942ms\nStdDev 0.164ms\nMedian 6.886ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.676700000047276,
            "unit": "ms",
            "extra": "Min 7.540ms\nMax 7.897ms\nMean 7.686ms\nStdDev 0.139ms\nMedian 7.677ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.032899999980145,
            "unit": "ms",
            "extra": "Min 17.445ms\nMax 19.452ms\nMean 18.362ms\nStdDev 0.987ms\nMedian 18.033ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00969999996414117,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.32079999982670415,
            "unit": "ms",
            "extra": "Min 0.315ms\nMax 0.350ms\nMean 0.327ms\nStdDev 0.015ms\nMedian 0.321ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.429599999975835,
            "unit": "ms",
            "extra": "Min 9.310ms\nMax 11.453ms\nMean 9.877ms\nStdDev 0.898ms\nMedian 9.430ms"
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
          "id": "50d3e9146eb3897865fc6f02fe4a8390f890b8cc",
          "message": "test: assert crashpad attachment upload (#1222)",
          "timestamp": "2025-05-05T16:12:02+02:00",
          "tree_id": "d1d5b1e80a85af14327828e096e1f9deb468c3ca",
          "url": "https://github.com/getsentry/sentry-native/commit/50d3e9146eb3897865fc6f02fe4a8390f890b8cc"
        },
        "date": 1746454649012,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.028499999933047,
            "unit": "ms",
            "extra": "Min 6.859ms\nMax 7.280ms\nMean 7.014ms\nStdDev 0.171ms\nMedian 7.028ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.320800000002237,
            "unit": "ms",
            "extra": "Min 7.048ms\nMax 7.499ms\nMean 7.290ms\nStdDev 0.192ms\nMedian 7.321ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.791499999953885,
            "unit": "ms",
            "extra": "Min 16.736ms\nMax 18.508ms\nMean 17.310ms\nStdDev 0.794ms\nMedian 16.791ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3113000000212196,
            "unit": "ms",
            "extra": "Min 0.298ms\nMax 0.334ms\nMean 0.312ms\nStdDev 0.015ms\nMedian 0.311ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.21440000001894,
            "unit": "ms",
            "extra": "Min 8.923ms\nMax 9.736ms\nMean 9.253ms\nStdDev 0.321ms\nMedian 9.214ms"
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
          "id": "5dd6782d06a110095cd51d3f45df7e455c006ccf",
          "message": "chore: make update-test-discovery sort locale-independent (#1227)",
          "timestamp": "2025-05-07T08:59:35+02:00",
          "tree_id": "bd47ca4133e3657c4e31cdf5eb9059381bae35f8",
          "url": "https://github.com/getsentry/sentry-native/commit/5dd6782d06a110095cd51d3f45df7e455c006ccf"
        },
        "date": 1746601439742,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.991500000026463,
            "unit": "ms",
            "extra": "Min 6.928ms\nMax 9.728ms\nMean 7.612ms\nStdDev 1.204ms\nMedian 6.992ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.200900000043475,
            "unit": "ms",
            "extra": "Min 7.117ms\nMax 7.328ms\nMean 7.228ms\nStdDev 0.086ms\nMedian 7.201ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.139700000029734,
            "unit": "ms",
            "extra": "Min 16.741ms\nMax 17.712ms\nMean 17.160ms\nStdDev 0.373ms\nMedian 17.140ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30850000007376366,
            "unit": "ms",
            "extra": "Min 0.289ms\nMax 0.325ms\nMean 0.307ms\nStdDev 0.015ms\nMedian 0.309ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.108299999979863,
            "unit": "ms",
            "extra": "Min 8.889ms\nMax 9.189ms\nMean 9.082ms\nStdDev 0.119ms\nMedian 9.108ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "8755115b8c37cc98ebb79dd518195de84164c847",
          "message": "feat: add `sentry_value_new_user` (#1228)\n\n* add sentry_value_new_user\n\n* add test\n\n* add comment\n\n* add test scenario\n\n* CHANGELOG.md\n\n* update user.id to be string\n\n* cleanup + add new_user_n implementation\n\n* use string_n instead\n\n* changelog formatting",
          "timestamp": "2025-05-07T17:40:39+02:00",
          "tree_id": "0e486f1c2446d75eeec9b866bd35b1e358ead766",
          "url": "https://github.com/getsentry/sentry-native/commit/8755115b8c37cc98ebb79dd518195de84164c847"
        },
        "date": 1746632745840,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.8111999999018735,
            "unit": "ms",
            "extra": "Min 6.793ms\nMax 7.090ms\nMean 6.902ms\nStdDev 0.137ms\nMedian 6.811ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.555200000069817,
            "unit": "ms",
            "extra": "Min 7.368ms\nMax 7.906ms\nMean 7.586ms\nStdDev 0.197ms\nMedian 7.555ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.98290000013003,
            "unit": "ms",
            "extra": "Min 16.782ms\nMax 17.327ms\nMean 17.058ms\nStdDev 0.219ms\nMedian 16.983ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009099999942918657,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.009ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3165000000535656,
            "unit": "ms",
            "extra": "Min 0.296ms\nMax 0.340ms\nMean 0.313ms\nStdDev 0.018ms\nMedian 0.317ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.031800000002477,
            "unit": "ms",
            "extra": "Min 8.803ms\nMax 9.862ms\nMean 9.125ms\nStdDev 0.431ms\nMedian 9.032ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "6349682+vaind@users.noreply.github.com",
            "name": "Ivan Dlugos",
            "username": "vaind"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "ba3d4167ff408f11245c986802ccf5ba6c073f31",
          "message": "chore: PS downstream SDK support (#1224)",
          "timestamp": "2025-05-13T07:38:49+02:00",
          "tree_id": "668de9b24f4bf4be9c3dc63a2d640d138116e24b",
          "url": "https://github.com/getsentry/sentry-native/commit/ba3d4167ff408f11245c986802ccf5ba6c073f31"
        },
        "date": 1747115013983,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.463799999982257,
            "unit": "ms",
            "extra": "Min 7.207ms\nMax 7.678ms\nMean 7.454ms\nStdDev 0.204ms\nMedian 7.464ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.608099999970364,
            "unit": "ms",
            "extra": "Min 7.504ms\nMax 8.239ms\nMean 7.795ms\nStdDev 0.337ms\nMedian 7.608ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.959300000029543,
            "unit": "ms",
            "extra": "Min 17.331ms\nMax 18.137ms\nMean 17.773ms\nStdDev 0.364ms\nMedian 17.959ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.016899999991437653,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.022ms\nMean 0.017ms\nStdDev 0.003ms\nMedian 0.017ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3490999999939959,
            "unit": "ms",
            "extra": "Min 0.303ms\nMax 0.367ms\nMean 0.335ms\nStdDev 0.029ms\nMedian 0.349ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.627099999988786,
            "unit": "ms",
            "extra": "Min 9.282ms\nMax 10.011ms\nMean 9.666ms\nStdDev 0.267ms\nMedian 9.627ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "bbca52a36c6811c5c18882099cb7e27fd10621d2",
          "message": "fix: trace sync improvements (#1200)\n\n* make set_trace write into propagation_context\n\n* check propagation context on transaction creation\n\n* clone instead of steal the propagation context trace data\n\n* apply propagation context when scoping transaction/span\n\n* progress\n\n* no longer apply trace data on span scoping\n\n* populate propagation_context with random trace_id and span_id\n\n* merge propagation context into contexts for event\n\n* add tests\n\n* only set propagation_context for TwP\n\n* finish all started spans\n\n* always init propagation_context\n\n* cleanup + CHANGELOG.md\n\n* extract init into static helper\n\n* first integration tests\n\n* more integration tests\n\n* remove todo\n\n* mark changes as breaking\n\n* CHANGELOG.md\n\n* Update CHANGELOG.md\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-13T11:26:04+02:00",
          "tree_id": "24f34cc1d128baf724703ef01f788002eac444d7",
          "url": "https://github.com/getsentry/sentry-native/commit/bbca52a36c6811c5c18882099cb7e27fd10621d2"
        },
        "date": 1747128654193,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.192200000019966,
            "unit": "ms",
            "extra": "Min 7.120ms\nMax 8.526ms\nMean 7.441ms\nStdDev 0.608ms\nMedian 7.192ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.21409999999878,
            "unit": "ms",
            "extra": "Min 7.115ms\nMax 7.359ms\nMean 7.229ms\nStdDev 0.110ms\nMedian 7.214ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.525999999975284,
            "unit": "ms",
            "extra": "Min 17.280ms\nMax 19.634ms\nMean 18.080ms\nStdDev 1.006ms\nMedian 17.526ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009700000077828008,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.29819999997471314,
            "unit": "ms",
            "extra": "Min 0.292ms\nMax 0.313ms\nMean 0.301ms\nStdDev 0.010ms\nMedian 0.298ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.557399999948757,
            "unit": "ms",
            "extra": "Min 9.311ms\nMax 10.111ms\nMean 9.646ms\nStdDev 0.336ms\nMedian 9.557ms"
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
          "id": "9a334b07e2e57472d92f4532c632339d0085402a",
          "message": "fix: support musl on Linux (#1233)\n\n* fix: libunwind as the macOS unwinder\n\n* fix: use libunwind with musl\n\n* ci: experiment with Alpine Linux (musl)\n\n* chore: comment out debug output\n\n* chore: update external/crashpad\n\n* fixup: don't set SENTRY_WITH_LIBUNWIND on APPLE\n\n* chore: find libunwind.h & libunwind.so\n\n* ci: libunwind vs. llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: update external/crashpad\n\n* chore: clean up\n\n* chore: update CHANGELOG.md\n\n* ci: gcc+libunwind vs. clang+llvm-libunwind\n\n* chore: update external/crashpad\n\n* chore: clean up extra newlines\n\n* build: pick libunwind.a when SENTRY_BUILD_SHARED_LIBS=0\n\n* build: add SENTRY_LIBUNWIND_SHARED option\n\n* build: lzma\n\n* drop llvm-libunwind\n\n* Update src/unwinder/sentry_unwinder_libunwind.c\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* chore: fix formatting\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-05-14T10:29:40+02:00",
          "tree_id": "f7ace77739aaf925a1fe60fadfb7e08f340feb1d",
          "url": "https://github.com/getsentry/sentry-native/commit/9a334b07e2e57472d92f4532c632339d0085402a"
        },
        "date": 1747211691083,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.951699999945049,
            "unit": "ms",
            "extra": "Min 6.810ms\nMax 7.111ms\nMean 6.962ms\nStdDev 0.110ms\nMedian 6.952ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.731300000045849,
            "unit": "ms",
            "extra": "Min 7.086ms\nMax 8.833ms\nMean 7.761ms\nStdDev 0.657ms\nMedian 7.731ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.80579999992915,
            "unit": "ms",
            "extra": "Min 16.430ms\nMax 17.129ms\nMean 16.797ms\nStdDev 0.302ms\nMedian 16.806ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009399999953529914,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.29879999999593565,
            "unit": "ms",
            "extra": "Min 0.296ms\nMax 0.311ms\nMean 0.302ms\nStdDev 0.007ms\nMedian 0.299ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.031399999912537,
            "unit": "ms",
            "extra": "Min 8.893ms\nMax 9.124ms\nMean 9.012ms\nStdDev 0.089ms\nMedian 9.031ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "committer": {
            "email": "bot@getsentry.com",
            "name": "getsentry-bot"
          },
          "distinct": true,
          "id": "5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a",
          "message": "Merge branch 'release/0.8.5'",
          "timestamp": "2025-05-14T13:10:30Z",
          "tree_id": "6d1f156d2a478609d441d118330310920630c4ec",
          "url": "https://github.com/getsentry/sentry-native/commit/5a2fd9d11ab8e75cf1aee471ce6e23eb6ed4eb1a"
        },
        "date": 1747228522691,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.373000000005959,
            "unit": "ms",
            "extra": "Min 7.901ms\nMax 8.604ms\nMean 8.318ms\nStdDev 0.282ms\nMedian 8.373ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.501899999941998,
            "unit": "ms",
            "extra": "Min 8.183ms\nMax 8.800ms\nMean 8.486ms\nStdDev 0.286ms\nMedian 8.502ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.98500000004333,
            "unit": "ms",
            "extra": "Min 18.463ms\nMax 19.359ms\nMean 18.912ms\nStdDev 0.348ms\nMedian 18.985ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013700000067728979,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.015ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.014ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3308999999944717,
            "unit": "ms",
            "extra": "Min 0.319ms\nMax 0.366ms\nMean 0.340ms\nStdDev 0.020ms\nMedian 0.331ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.215200000061486,
            "unit": "ms",
            "extra": "Min 10.153ms\nMax 10.681ms\nMean 10.375ms\nStdDev 0.260ms\nMedian 10.215ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "mischan@abovevacant.com",
            "name": "Mischan Toosarani-Hausberger",
            "username": "supervacuus"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "2c2ef4479c2717cc2055c3939d5bbab675ab6a69",
          "message": "chore: bump codecov-action to 5.4.2 (#1242)",
          "timestamp": "2025-05-14T18:23:48+02:00",
          "tree_id": "24e5b4492ef5466d2b194fc634299667d390ba7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2c2ef4479c2717cc2055c3939d5bbab675ab6a69"
        },
        "date": 1747240108212,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.288600000038059,
            "unit": "ms",
            "extra": "Min 6.866ms\nMax 7.475ms\nMean 7.187ms\nStdDev 0.262ms\nMedian 7.289ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.2843999998895015,
            "unit": "ms",
            "extra": "Min 7.124ms\nMax 7.464ms\nMean 7.271ms\nStdDev 0.132ms\nMedian 7.284ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.279900000062298,
            "unit": "ms",
            "extra": "Min 17.043ms\nMax 17.525ms\nMean 17.303ms\nStdDev 0.179ms\nMedian 17.280ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3153000000111206,
            "unit": "ms",
            "extra": "Min 0.294ms\nMax 0.378ms\nMean 0.323ms\nStdDev 0.033ms\nMedian 0.315ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.22549999995681,
            "unit": "ms",
            "extra": "Min 8.844ms\nMax 9.577ms\nMean 9.257ms\nStdDev 0.291ms\nMedian 9.225ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "60878493+JoshuaMoelans@users.noreply.github.com",
            "name": "JoshuaMoelans",
            "username": "JoshuaMoelans"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "5224a83ca41207085f0694caca460bb33852efb0",
          "message": "feat: Add `before_send_transaction` (#1236)\n\n* initial before_send_transaction implementation\n\n* CHANGELOG.md\n\n* add before_transaction_data\n\n* add tests\n\n* rename `closure` to `user_data`\n\n* update CHANGELOG.md after merging main",
          "timestamp": "2025-05-15T12:26:44+02:00",
          "tree_id": "edf2411e42a743cd55bf1a5399e9ec0959b7c8d1",
          "url": "https://github.com/getsentry/sentry-native/commit/5224a83ca41207085f0694caca460bb33852efb0"
        },
        "date": 1747305103177,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.756499999994503,
            "unit": "ms",
            "extra": "Min 6.678ms\nMax 7.132ms\nMean 6.858ms\nStdDev 0.213ms\nMedian 6.756ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.030799999995452,
            "unit": "ms",
            "extra": "Min 6.897ms\nMax 7.194ms\nMean 7.022ms\nStdDev 0.112ms\nMedian 7.031ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.703499999981887,
            "unit": "ms",
            "extra": "Min 16.858ms\nMax 19.633ms\nMean 17.952ms\nStdDev 1.026ms\nMedian 17.703ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012500000025283953,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.29359999996358965,
            "unit": "ms",
            "extra": "Min 0.286ms\nMax 0.301ms\nMean 0.295ms\nStdDev 0.006ms\nMedian 0.294ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.243299999980081,
            "unit": "ms",
            "extra": "Min 9.159ms\nMax 9.828ms\nMean 9.349ms\nStdDev 0.272ms\nMedian 9.243ms"
          }
        ]
      }
    ]
  }
}