window.BENCHMARK_DATA = {
  "lastUpdate": 1754656924038,
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
          "id": "426c60558d68ac13204d9c9bfb64f5284cb36d7e",
          "message": "docs: windows test and formatter runners (#1247)",
          "timestamp": "2025-05-15T17:27:13+02:00",
          "tree_id": "7a79c6dc37e714afa484243ab15e43e50d3a99fb",
          "url": "https://github.com/getsentry/sentry-native/commit/426c60558d68ac13204d9c9bfb64f5284cb36d7e"
        },
        "date": 1747322966181,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7351190000122187,
            "unit": "ms",
            "extra": "Min 0.711ms\nMax 0.781ms\nMean 0.741ms\nStdDev 0.031ms\nMedian 0.735ms\nCPU 0.741ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7166850000430713,
            "unit": "ms",
            "extra": "Min 0.706ms\nMax 0.734ms\nMean 0.719ms\nStdDev 0.011ms\nMedian 0.717ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9308389999869178,
            "unit": "ms",
            "extra": "Min 2.885ms\nMax 3.245ms\nMean 2.995ms\nStdDev 0.145ms\nMedian 2.931ms\nCPU 1.501ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012222999998812156,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022060999981476925,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7654800000173054,
            "unit": "ms",
            "extra": "Min 1.742ms\nMax 1.870ms\nMean 1.788ms\nStdDev 0.053ms\nMedian 1.765ms\nCPU 0.568ms"
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
          "id": "c653de396546b870f563f320f15362282a2432b5",
          "message": "chore: update gradle scripts to current AGP/SDK usage (#1256)",
          "timestamp": "2025-05-27T15:08:13+02:00",
          "tree_id": "f3df760bb7c0e2f55580d7011626ef29a7b77cff",
          "url": "https://github.com/getsentry/sentry-native/commit/c653de396546b870f563f320f15362282a2432b5"
        },
        "date": 1748351438684,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7662099999947714,
            "unit": "ms",
            "extra": "Min 0.740ms\nMax 0.853ms\nMean 0.777ms\nStdDev 0.045ms\nMedian 0.766ms\nCPU 0.767ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7792150000227593,
            "unit": "ms",
            "extra": "Min 0.728ms\nMax 1.077ms\nMean 0.833ms\nStdDev 0.140ms\nMedian 0.779ms\nCPU 0.809ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.2964119999974173,
            "unit": "ms",
            "extra": "Min 3.179ms\nMax 16.782ms\nMean 5.975ms\nStdDev 6.041ms\nMedian 3.296ms\nCPU 1.712ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013384999988375057,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.025447999973948754,
            "unit": "ms",
            "extra": "Min 0.024ms\nMax 0.034ms\nMean 0.027ms\nStdDev 0.004ms\nMedian 0.025ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 2.043678000006821,
            "unit": "ms",
            "extra": "Min 1.927ms\nMax 2.130ms\nMean 2.017ms\nStdDev 0.087ms\nMedian 2.044ms\nCPU 0.612ms"
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
          "id": "aa1f1b79dbf31d9576dd9573e8c863dd685ea13b",
          "message": "fix(ndk): correct interpolation of new `project.layout.buildDirectory` property (#1258)",
          "timestamp": "2025-05-28T10:09:46+02:00",
          "tree_id": "4245ae38cbb06385b4a7e795849313dfe997dac8",
          "url": "https://github.com/getsentry/sentry-native/commit/aa1f1b79dbf31d9576dd9573e8c863dd685ea13b"
        },
        "date": 1748419929580,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7081070000367617,
            "unit": "ms",
            "extra": "Min 0.698ms\nMax 0.725ms\nMean 0.709ms\nStdDev 0.011ms\nMedian 0.708ms\nCPU 0.709ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7174049999889576,
            "unit": "ms",
            "extra": "Min 0.707ms\nMax 0.757ms\nMean 0.727ms\nStdDev 0.020ms\nMedian 0.717ms\nCPU 0.727ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.45221500003845,
            "unit": "ms",
            "extra": "Min 2.872ms\nMax 3.801ms\nMean 3.334ms\nStdDev 0.401ms\nMedian 3.452ms\nCPU 1.514ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01209300000937219,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02285300001858559,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.810174000013376,
            "unit": "ms",
            "extra": "Min 1.778ms\nMax 1.954ms\nMean 1.849ms\nStdDev 0.077ms\nMedian 1.810ms\nCPU 0.563ms"
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
          "id": "6ca83891fdea3f141df66eb6a2f148ef465463c2",
          "message": "fix: use custom page allocator on ps5 (#1257)",
          "timestamp": "2025-05-28T10:16:54+02:00",
          "tree_id": "839d7d4472a60b05ccb61e0105979c5205dd1bf1",
          "url": "https://github.com/getsentry/sentry-native/commit/6ca83891fdea3f141df66eb6a2f148ef465463c2"
        },
        "date": 1748420357688,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7306059999905301,
            "unit": "ms",
            "extra": "Min 0.711ms\nMax 0.847ms\nMean 0.748ms\nStdDev 0.056ms\nMedian 0.731ms\nCPU 0.733ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7282710000140469,
            "unit": "ms",
            "extra": "Min 0.698ms\nMax 0.772ms\nMean 0.733ms\nStdDev 0.027ms\nMedian 0.728ms\nCPU 0.733ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9783520000137287,
            "unit": "ms",
            "extra": "Min 2.932ms\nMax 3.051ms\nMean 2.989ms\nStdDev 0.049ms\nMedian 2.978ms\nCPU 1.544ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012483999967116688,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022882999985540664,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9293639999773404,
            "unit": "ms",
            "extra": "Min 1.854ms\nMax 1.956ms\nMean 1.921ms\nStdDev 0.041ms\nMedian 1.929ms\nCPU 0.592ms"
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
          "id": "a1b3947584935eea5680ba54e8914ec913509566",
          "message": "fix: limit proguard rules in the NDK package to local namespaces (#1250)\n\n* fix: limit proguard rules in the NDK package to local namespaces\n\n* rewörding",
          "timestamp": "2025-06-02T08:08:51+02:00",
          "tree_id": "ad71de44e8f71235e89c6e97cd355467f52ddf91",
          "url": "https://github.com/getsentry/sentry-native/commit/a1b3947584935eea5680ba54e8914ec913509566"
        },
        "date": 1748844664724,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7110619999366463,
            "unit": "ms",
            "extra": "Min 0.697ms\nMax 0.812ms\nMean 0.728ms\nStdDev 0.048ms\nMedian 0.711ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7137360000797344,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.722ms\nMean 0.711ms\nStdDev 0.012ms\nMedian 0.714ms\nCPU 0.709ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.0319090000148208,
            "unit": "ms",
            "extra": "Min 2.908ms\nMax 3.509ms\nMean 3.106ms\nStdDev 0.237ms\nMedian 3.032ms\nCPU 1.498ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013014000046496221,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.019ms\nMean 0.014ms\nStdDev 0.003ms\nMedian 0.013ms\nCPU 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022252000007938477,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8279000000802625,
            "unit": "ms",
            "extra": "Min 1.765ms\nMax 1.923ms\nMean 1.847ms\nStdDev 0.065ms\nMedian 1.828ms\nCPU 0.581ms"
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
          "id": "9c0950343b3531c797b4c1c87ae6f5b5d1281ed3",
          "message": "fix: close file and return 0 on success when writing raw envelopes (#1260)",
          "timestamp": "2025-06-02T10:54:33+02:00",
          "tree_id": "2d6515f02317a16a2f5e417fe7850a1042cefa60",
          "url": "https://github.com/getsentry/sentry-native/commit/9c0950343b3531c797b4c1c87ae6f5b5d1281ed3"
        },
        "date": 1748854700076,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.6963129999917328,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.721ms\nMean 0.703ms\nStdDev 0.012ms\nMedian 0.696ms\nCPU 0.702ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.8474539999951958,
            "unit": "ms",
            "extra": "Min 0.816ms\nMax 0.947ms\nMean 0.863ms\nStdDev 0.050ms\nMedian 0.847ms\nCPU 0.862ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.0124379999847406,
            "unit": "ms",
            "extra": "Min 2.963ms\nMax 3.085ms\nMean 3.028ms\nStdDev 0.052ms\nMedian 3.012ms\nCPU 1.596ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01249299998562492,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.015ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022642000033101795,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9733329999667149,
            "unit": "ms",
            "extra": "Min 1.930ms\nMax 1.986ms\nMean 1.968ms\nStdDev 0.022ms\nMedian 1.973ms\nCPU 0.609ms"
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
          "id": "0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a",
          "message": "ci: pin kcov to v43 (#1265)",
          "timestamp": "2025-06-03T10:19:39+02:00",
          "tree_id": "71da9cd13983c4e189589ad750e36b9fe96bccbc",
          "url": "https://github.com/getsentry/sentry-native/commit/0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a"
        },
        "date": 1748938991672,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7143580000388283,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.729ms\nMean 0.711ms\nStdDev 0.015ms\nMedian 0.714ms\nCPU 0.711ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7142460000295614,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.730ms\nMean 0.715ms\nStdDev 0.015ms\nMedian 0.714ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9116360000216446,
            "unit": "ms",
            "extra": "Min 2.846ms\nMax 3.000ms\nMean 2.921ms\nStdDev 0.056ms\nMedian 2.912ms\nCPU 1.499ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012432999994871352,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02176099997086567,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.024ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7481190000125935,
            "unit": "ms",
            "extra": "Min 1.724ms\nMax 1.816ms\nMean 1.755ms\nStdDev 0.037ms\nMedian 1.748ms\nCPU 0.552ms"
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
          "id": "1fe91217f1ce839f46ef398bdf1f3bf58d99a0da",
          "message": "ci: use Alpine Linux Docker image (#1261)",
          "timestamp": "2025-06-03T11:10:02+02:00",
          "tree_id": "2a2a6c8a0fff494a2d8eb478d6f7350cc26c2125",
          "url": "https://github.com/getsentry/sentry-native/commit/1fe91217f1ce839f46ef398bdf1f3bf58d99a0da"
        },
        "date": 1748941935487,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7449999999948886,
            "unit": "ms",
            "extra": "Min 0.710ms\nMax 0.836ms\nMean 0.762ms\nStdDev 0.056ms\nMedian 0.745ms\nCPU 0.745ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7078699999851779,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.752ms\nMean 0.716ms\nStdDev 0.021ms\nMedian 0.708ms\nCPU 0.716ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.393831999972008,
            "unit": "ms",
            "extra": "Min 3.210ms\nMax 3.521ms\nMean 3.363ms\nStdDev 0.124ms\nMedian 3.394ms\nCPU 1.716ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012273000038476312,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022422000029109768,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.039ms\nMean 0.026ms\nStdDev 0.007ms\nMedian 0.022ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9151179999994383,
            "unit": "ms",
            "extra": "Min 1.859ms\nMax 1.922ms\nMean 1.904ms\nStdDev 0.026ms\nMedian 1.915ms\nCPU 0.600ms"
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
          "id": "fc6a314033b1df0ddb9a9ecb51b3268deb7572cb",
          "message": "fix: respect event data when applying/merging scope data (#1253)",
          "timestamp": "2025-06-04T12:32:03+02:00",
          "tree_id": "04848bc8968f055481f86c213cbd3a2f3e6dafa1",
          "url": "https://github.com/getsentry/sentry-native/commit/fc6a314033b1df0ddb9a9ecb51b3268deb7572cb"
        },
        "date": 1749033259145,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7149749999939559,
            "unit": "ms",
            "extra": "Min 0.677ms\nMax 0.767ms\nMean 0.716ms\nStdDev 0.039ms\nMedian 0.715ms\nCPU 0.711ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7144150000044647,
            "unit": "ms",
            "extra": "Min 0.712ms\nMax 0.788ms\nMean 0.731ms\nStdDev 0.032ms\nMedian 0.714ms\nCPU 0.719ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.859738000040579,
            "unit": "ms",
            "extra": "Min 2.830ms\nMax 2.920ms\nMean 2.871ms\nStdDev 0.036ms\nMedian 2.860ms\nCPU 1.490ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012352999988252122,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022150999996028986,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.026ms\nMean 0.023ms\nStdDev 0.002ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7852729999958683,
            "unit": "ms",
            "extra": "Min 1.717ms\nMax 1.892ms\nMean 1.808ms\nStdDev 0.072ms\nMedian 1.785ms\nCPU 0.546ms"
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
          "id": "31a8ee35ab3a77fcd201535ea814a3611ad34102",
          "message": "feat: add PS SDK transport support (#1262)\n\n* feat: allow downstream SDKs to implement custom transport\n\n* add \"pshttp\" support to the list of supported ones",
          "timestamp": "2025-06-04T14:10:02+02:00",
          "tree_id": "397dcdefbc9b7cc6aafa6f596959832e93704850",
          "url": "https://github.com/getsentry/sentry-native/commit/31a8ee35ab3a77fcd201535ea814a3611ad34102"
        },
        "date": 1749039196558,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7518120000042927,
            "unit": "ms",
            "extra": "Min 0.729ms\nMax 0.787ms\nMean 0.751ms\nStdDev 0.023ms\nMedian 0.752ms\nCPU 0.750ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7103350000079445,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.781ms\nMean 0.721ms\nStdDev 0.035ms\nMedian 0.710ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.1248139999888735,
            "unit": "ms",
            "extra": "Min 2.944ms\nMax 3.333ms\nMean 3.125ms\nStdDev 0.158ms\nMedian 3.125ms\nCPU 1.577ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012353000045095541,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022651999984191207,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7771920000200225,
            "unit": "ms",
            "extra": "Min 1.762ms\nMax 1.821ms\nMean 1.782ms\nStdDev 0.023ms\nMedian 1.777ms\nCPU 0.549ms"
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
          "id": "54cede931f9a76e291be9083d56702c7d20150cd",
          "message": "feat: Add support for capturing events with local scopes (#1248)\n\n* wip: local scopes\n\n* merge breadcrumbs\n\n* add sentry_scope_set_trace\n\n* add sentry_scope_set_fingerprints()\n\n* check fingerprints value type\n\n* document sentry_scope_set_fingerprints() expected type\n\n* Revert sentry_scope_set_trace/transaction\n\n> Transactions/spans do not make sense in this setup since they aren't\n> cloned and cannot be retrieved to create children.\n\n* sentry_malloc -> SENTRY_MAKE\n\n* fix comparing null timestamps when merging breadcrumbs\n\n* take ownership\n\n* update example\n\n* partial revert of unit test changes in a48fea\n\ndon't assume any specific order for breadcrumbs with missing breadcrumbs\n\n* warn once if any breadcrumbs were missing timestamps\n\n* error handling for sentry_value_append()",
          "timestamp": "2025-06-04T16:29:38+02:00",
          "tree_id": "2c4e06d37f7f702c92c3effcb198b108f06a6b70",
          "url": "https://github.com/getsentry/sentry-native/commit/54cede931f9a76e291be9083d56702c7d20150cd"
        },
        "date": 1749047519825,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7159770000271237,
            "unit": "ms",
            "extra": "Min 0.708ms\nMax 0.749ms\nMean 0.722ms\nStdDev 0.017ms\nMedian 0.716ms\nCPU 0.722ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7145550000018375,
            "unit": "ms",
            "extra": "Min 0.703ms\nMax 0.793ms\nMean 0.729ms\nStdDev 0.037ms\nMedian 0.715ms\nCPU 0.729ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9981139999790685,
            "unit": "ms",
            "extra": "Min 2.913ms\nMax 3.154ms\nMean 3.034ms\nStdDev 0.097ms\nMedian 2.998ms\nCPU 1.555ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012212000001454726,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022342000022490538,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.85701599997401,
            "unit": "ms",
            "extra": "Min 1.830ms\nMax 2.468ms\nMean 1.979ms\nStdDev 0.275ms\nMedian 1.857ms\nCPU 0.604ms"
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
          "id": "c56ebdea24486771e28853b78afcd4f6d8ab4833",
          "message": "feat: `crashpad_wait_for_upload` Windows support (#1255)",
          "timestamp": "2025-06-05T18:46:32+02:00",
          "tree_id": "2b213da0b6f44cb2588ac5cd94f4cf3006cbccf2",
          "url": "https://github.com/getsentry/sentry-native/commit/c56ebdea24486771e28853b78afcd4f6d8ab4833"
        },
        "date": 1749142141543,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7095490000210702,
            "unit": "ms",
            "extra": "Min 0.689ms\nMax 0.727ms\nMean 0.706ms\nStdDev 0.016ms\nMedian 0.710ms\nCPU 0.706ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7790789999830849,
            "unit": "ms",
            "extra": "Min 0.751ms\nMax 0.886ms\nMean 0.801ms\nStdDev 0.058ms\nMedian 0.779ms\nCPU 0.790ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9775190000123075,
            "unit": "ms",
            "extra": "Min 2.938ms\nMax 3.182ms\nMean 3.025ms\nStdDev 0.104ms\nMedian 2.978ms\nCPU 1.524ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012544000014713674,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022451999996064842,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8205279999961022,
            "unit": "ms",
            "extra": "Min 1.751ms\nMax 1.890ms\nMean 1.819ms\nStdDev 0.051ms\nMedian 1.821ms\nCPU 0.582ms"
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
          "id": "9b29c7996e06c200c23c199d95844be8366b16cc",
          "message": "Merge branch 'release/0.9.0'",
          "timestamp": "2025-06-05T17:31:05Z",
          "tree_id": "522307788bf54fcc5ce2e6fa66c7c423134a840c",
          "url": "https://github.com/getsentry/sentry-native/commit/9b29c7996e06c200c23c199d95844be8366b16cc"
        },
        "date": 1749144796246,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7078950000050099,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.719ms\nMean 0.706ms\nStdDev 0.013ms\nMedian 0.708ms\nCPU 0.705ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7074559999864505,
            "unit": "ms",
            "extra": "Min 0.696ms\nMax 0.719ms\nMean 0.709ms\nStdDev 0.009ms\nMedian 0.707ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9123150000032183,
            "unit": "ms",
            "extra": "Min 2.888ms\nMax 3.108ms\nMean 2.965ms\nStdDev 0.098ms\nMedian 2.912ms\nCPU 1.502ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012212999990879325,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02278299999147748,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.789285000000973,
            "unit": "ms",
            "extra": "Min 1.728ms\nMax 1.803ms\nMean 1.776ms\nStdDev 0.031ms\nMedian 1.789ms\nCPU 0.553ms"
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
          "id": "16fa6892134f162ddbfaaa94b230ca5cd8564e0a",
          "message": "fix: introduce malloc/MAKE rv checks if missing (#1234)\n\n+ ensure that none of the test runs into a segfault by asserting on malloc return paths that propagate",
          "timestamp": "2025-06-12T13:29:34+02:00",
          "tree_id": "f8c1c235ae6bbe2d43730436229d5ee7e5ffc7e2",
          "url": "https://github.com/getsentry/sentry-native/commit/16fa6892134f162ddbfaaa94b230ca5cd8564e0a"
        },
        "date": 1749727906281,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7050809999782359,
            "unit": "ms",
            "extra": "Min 0.687ms\nMax 0.786ms\nMean 0.722ms\nStdDev 0.042ms\nMedian 0.705ms\nCPU 0.710ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.703616000009788,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.721ms\nMean 0.703ms\nStdDev 0.013ms\nMedian 0.704ms\nCPU 0.702ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8552020000063294,
            "unit": "ms",
            "extra": "Min 2.770ms\nMax 3.010ms\nMean 2.875ms\nStdDev 0.087ms\nMedian 2.855ms\nCPU 1.504ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012291999979652246,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.0226820000079897,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.769587000012507,
            "unit": "ms",
            "extra": "Min 1.726ms\nMax 1.787ms\nMean 1.763ms\nStdDev 0.024ms\nMedian 1.770ms\nCPU 0.550ms"
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
          "id": "1bf8db1646a8e6d3b497f3d173368192b75e05ba",
          "message": "ci: drop windows-2019 runner images (#1274)",
          "timestamp": "2025-06-13T11:51:12+02:00",
          "tree_id": "732b83a0139c34a3b7fdfcb4247c8600cd7022b6",
          "url": "https://github.com/getsentry/sentry-native/commit/1bf8db1646a8e6d3b497f3d173368192b75e05ba"
        },
        "date": 1749808413665,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7053629999518307,
            "unit": "ms",
            "extra": "Min 0.689ms\nMax 0.781ms\nMean 0.719ms\nStdDev 0.036ms\nMedian 0.705ms\nCPU 0.713ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7118750000358887,
            "unit": "ms",
            "extra": "Min 0.694ms\nMax 0.809ms\nMean 0.734ms\nStdDev 0.046ms\nMedian 0.712ms\nCPU 0.728ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9928480000762647,
            "unit": "ms",
            "extra": "Min 2.872ms\nMax 3.166ms\nMean 3.009ms\nStdDev 0.105ms\nMedian 2.993ms\nCPU 1.494ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012313999945945397,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022211999976207153,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7843840000750788,
            "unit": "ms",
            "extra": "Min 1.749ms\nMax 1.873ms\nMean 1.803ms\nStdDev 0.052ms\nMedian 1.784ms\nCPU 0.556ms"
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
          "id": "7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8",
          "message": "feat: Support modifying attachments after init (continued) (#1266)\n\n* feat: Support modifying attachments after init\n\nMoves the attachments to the scope, and adds `sentry_add_attachment` and\n`sentry_remove_attachment` and wstr variants that modify this attachment\nlist after calling init. Attachments are identified by their path.\n\n* feat: pass added and removed attachments to the backend\n\n* add `_n`\n\n* scope api\n\n* merge & apply attachments\n\n* update note on attachments\n\n* integration tests\n\n* Update README.md\n\n* Update CHANGELOG.md\n\n* Apply suggestions from code review\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* remove ticks\n\n* Apply more suggestions from code review\n\n* De-duplicate envelope attachment code\n\n- remove sentry__apply_attachments_to_envelope\n- add sentry__envelope_add_attachments\n- reuse sentry__envelope_add_attachment\n\n* sentry_add_attachment -> sentry_add_attachment_path\n\n* Update CHANGELOG.md\n\n* fixup: missed rename\n\n* fixup: another missed rename\n\n* remove_attachmentw() without _path\n\n* revise sentry_attach_file & removal\n\n* fix windows\n\n* Update CHANGELOG.md\n\n* clean up\n\n* fix attachments_add_remove on windows\n\n* Update CHANGELOG.md & NOTE on attachments\n\n* Update external/crashpad\n\n---------\n\nCo-authored-by: Arpad Borsos <arpad.borsos@googlemail.com>\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-06-13T14:02:33+02:00",
          "tree_id": "cb94ac9185e31f5e47c7ebc651235c14006bf020",
          "url": "https://github.com/getsentry/sentry-native/commit/7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8"
        },
        "date": 1749816302775,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7265870000026098,
            "unit": "ms",
            "extra": "Min 0.708ms\nMax 0.738ms\nMean 0.725ms\nStdDev 0.013ms\nMedian 0.727ms\nCPU 0.724ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7286900000167407,
            "unit": "ms",
            "extra": "Min 0.694ms\nMax 0.752ms\nMean 0.724ms\nStdDev 0.022ms\nMedian 0.729ms\nCPU 0.724ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.452856000023985,
            "unit": "ms",
            "extra": "Min 2.976ms\nMax 3.510ms\nMean 3.285ms\nStdDev 0.270ms\nMedian 3.453ms\nCPU 1.644ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012273000010054602,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.027ms\nMean 0.015ms\nStdDev 0.007ms\nMedian 0.012ms\nCPU 0.014ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02269200001592253,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8192629999873589,
            "unit": "ms",
            "extra": "Min 1.762ms\nMax 1.888ms\nMean 1.822ms\nStdDev 0.050ms\nMedian 1.819ms\nCPU 0.577ms"
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
          "id": "d823acbc116d19393c3552a123ab7e483c383b03",
          "message": "docs: sync return values of AIX dladdr with implementation (#1273)",
          "timestamp": "2025-06-13T15:22:47+02:00",
          "tree_id": "abde47443e4fe5f6a7a7f6e22cff13dfa090ffb9",
          "url": "https://github.com/getsentry/sentry-native/commit/d823acbc116d19393c3552a123ab7e483c383b03"
        },
        "date": 1749821100460,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7447059999776684,
            "unit": "ms",
            "extra": "Min 0.715ms\nMax 0.821ms\nMean 0.751ms\nStdDev 0.043ms\nMedian 0.745ms\nCPU 0.745ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7170730000041203,
            "unit": "ms",
            "extra": "Min 0.692ms\nMax 0.814ms\nMean 0.730ms\nStdDev 0.048ms\nMedian 0.717ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.009080999959224,
            "unit": "ms",
            "extra": "Min 2.824ms\nMax 3.688ms\nMean 3.081ms\nStdDev 0.350ms\nMedian 3.009ms\nCPU 1.527ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012422999986938521,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022332000014557707,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.049ms\nMean 0.028ms\nStdDev 0.012ms\nMedian 0.022ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8075410000051306,
            "unit": "ms",
            "extra": "Min 1.791ms\nMax 1.872ms\nMean 1.818ms\nStdDev 0.031ms\nMedian 1.808ms\nCPU 0.543ms"
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
          "id": "931c468cb6dcb8856c60cd113ef044d9881ef62e",
          "message": "test: define individual unit tests for CTEST (#1244)\n\n* test: define individual unit tests for CTEST\n\n* chore: add SENTRY_CTEST_INDIVIDUAL option\n\n* rename the aggregate test target to \"unit-tests\"",
          "timestamp": "2025-06-16T12:19:00+02:00",
          "tree_id": "a5600d12fbaa848014d06846b9b5d5ea7bf1a63a",
          "url": "https://github.com/getsentry/sentry-native/commit/931c468cb6dcb8856c60cd113ef044d9881ef62e"
        },
        "date": 1750069274716,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7305270000017572,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.756ms\nMean 0.728ms\nStdDev 0.024ms\nMedian 0.731ms\nCPU 0.728ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6980859999998756,
            "unit": "ms",
            "extra": "Min 0.686ms\nMax 0.753ms\nMean 0.708ms\nStdDev 0.028ms\nMedian 0.698ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.1155780000062805,
            "unit": "ms",
            "extra": "Min 2.948ms\nMax 3.280ms\nMean 3.107ms\nStdDev 0.140ms\nMedian 3.116ms\nCPU 1.511ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012734000023328917,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.034ms\nMean 0.017ms\nStdDev 0.010ms\nMedian 0.013ms\nCPU 0.016ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022481999991441626,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7768529999955263,
            "unit": "ms",
            "extra": "Min 1.737ms\nMax 1.845ms\nMean 1.789ms\nStdDev 0.045ms\nMedian 1.777ms\nCPU 0.578ms"
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
          "id": "455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8",
          "message": "feat: add sentry_attachment_set_content_type() (#1276)\n\n* feat: add sentry_attachment_set_content_type()\n\n* drop content_type_owned\n\n* add _n",
          "timestamp": "2025-06-16T15:55:01+02:00",
          "tree_id": "0af59a742a894ffc9893e993308fbf3fa7f147b9",
          "url": "https://github.com/getsentry/sentry-native/commit/455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8"
        },
        "date": 1750082250386,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7263829999999416,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.753ms\nMean 0.719ms\nStdDev 0.029ms\nMedian 0.726ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7263850000072125,
            "unit": "ms",
            "extra": "Min 0.705ms\nMax 0.753ms\nMean 0.726ms\nStdDev 0.018ms\nMedian 0.726ms\nCPU 0.726ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.2489199999758966,
            "unit": "ms",
            "extra": "Min 3.080ms\nMax 3.320ms\nMean 3.219ms\nStdDev 0.091ms\nMedian 3.249ms\nCPU 1.632ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012513000001490582,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.041ms\nMean 0.018ms\nStdDev 0.013ms\nMedian 0.013ms\nCPU 0.017ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022061000038320344,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.851859999987937,
            "unit": "ms",
            "extra": "Min 1.823ms\nMax 1.880ms\nMean 1.851ms\nStdDev 0.022ms\nMedian 1.852ms\nCPU 0.579ms"
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
          "id": "99e598206296b63301919b9bc20e36c39adb51a1",
          "message": "chore: `breakpad` upstream update (#1277)\n\n* update breakpad + lss\n\n* CHANGELOG.md\n\n* breakpad post-merge",
          "timestamp": "2025-06-17T12:42:03+02:00",
          "tree_id": "3bc4293c0013fb354981e4896e27aa66ea889b16",
          "url": "https://github.com/getsentry/sentry-native/commit/99e598206296b63301919b9bc20e36c39adb51a1"
        },
        "date": 1750157062156,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7076240000003509,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.721ms\nMean 0.708ms\nStdDev 0.013ms\nMedian 0.708ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7206879999728244,
            "unit": "ms",
            "extra": "Min 0.702ms\nMax 0.825ms\nMean 0.735ms\nStdDev 0.051ms\nMedian 0.721ms\nCPU 0.735ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.876054000012118,
            "unit": "ms",
            "extra": "Min 2.791ms\nMax 2.915ms\nMean 2.872ms\nStdDev 0.048ms\nMedian 2.876ms\nCPU 1.484ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012171999969723402,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02227199996696072,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7707900000232257,
            "unit": "ms",
            "extra": "Min 1.762ms\nMax 1.809ms\nMean 1.776ms\nStdDev 0.019ms\nMedian 1.771ms\nCPU 0.534ms"
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
          "id": "6aaccb486304e891170fd7489c4d2cf76f8bfc1d",
          "message": "docs: remove `sentry_event_value_add_stacktrace()` from example (#1281)",
          "timestamp": "2025-06-20T15:16:54+02:00",
          "tree_id": "5270710c3fb3f6847aba3bd3bade5a46ca262e5d",
          "url": "https://github.com/getsentry/sentry-native/commit/6aaccb486304e891170fd7489c4d2cf76f8bfc1d"
        },
        "date": 1750425622064,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.715367999987393,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.757ms\nMean 0.720ms\nStdDev 0.026ms\nMedian 0.715ms\nCPU 0.720ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7098180000184584,
            "unit": "ms",
            "extra": "Min 0.683ms\nMax 0.734ms\nMean 0.707ms\nStdDev 0.019ms\nMedian 0.710ms\nCPU 0.706ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.0616870000130803,
            "unit": "ms",
            "extra": "Min 2.986ms\nMax 3.120ms\nMean 3.056ms\nStdDev 0.049ms\nMedian 3.062ms\nCPU 1.565ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01250299999355775,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022051000030387513,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9244839999714713,
            "unit": "ms",
            "extra": "Min 1.824ms\nMax 2.000ms\nMean 1.905ms\nStdDev 0.073ms\nMedian 1.924ms\nCPU 0.573ms"
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
          "id": "889d59aa2d165850e7fcdbedcb9770144638874f",
          "message": "chore: xbox compilation fixes and cleanup (#1284)\n\n* fix: xbox compilation\n\n* replace checks of _GAMING_XBOX_SCARLETT with SENTRY_PLATFORM_XBOX_SCARLETT\n\n* test: skip tests for missing features",
          "timestamp": "2025-06-24T14:52:27+02:00",
          "tree_id": "c6bb64d8f4f63b2374351a0a9ea016cf2782b76e",
          "url": "https://github.com/getsentry/sentry-native/commit/889d59aa2d165850e7fcdbedcb9770144638874f"
        },
        "date": 1750769750800,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.701599000024089,
            "unit": "ms",
            "extra": "Min 0.678ms\nMax 0.709ms\nMean 0.698ms\nStdDev 0.012ms\nMedian 0.702ms\nCPU 0.698ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7303929999977754,
            "unit": "ms",
            "extra": "Min 0.718ms\nMax 0.744ms\nMean 0.732ms\nStdDev 0.010ms\nMedian 0.730ms\nCPU 0.731ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9250919999981306,
            "unit": "ms",
            "extra": "Min 2.903ms\nMax 3.087ms\nMean 2.954ms\nStdDev 0.076ms\nMedian 2.925ms\nCPU 1.511ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012091999991525881,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022212000004628862,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7865670000105638,
            "unit": "ms",
            "extra": "Min 1.757ms\nMax 1.813ms\nMean 1.789ms\nStdDev 0.023ms\nMedian 1.787ms\nCPU 0.566ms"
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
          "id": "9dd7d6c61a1a3406fc613b4e2bad40baacc5453f",
          "message": "feat: support attaching bytes (#1275)\n\n* feat: Support attaching bytes\n\n* fix: move to_crashpad_attachment out of extern C\n\n> warning C4190: 'to_crashpad_attachment' has C-linkage specified,\n> but returns UDT 'crashpad::Attachment' which is incompatible with C\n\n* fix: lint\n\n* fix: unreachable\n\n* test: integration\n\n* test: rename\n\n* test: attaching bytes to crashpad is supported on win32 and linux\n\n* crashpad: dump byte attachments on disk\n\n* fix: windows\n\n* let crashpad ensure unique file names\n\n* fix sentry__attachment_from_buffer\n\n* clean up unused uuid\n\n* Update external/crashpad\n\n* alternative: ensure unique file in sentry_backend_crashpad\n\n* clean up\n\n* clean up more\n\n* switch to std::filesystem\n\n* fix leaks in backends\n\n* add sentry__attachment_from_path for convenience and to reduce diff\n\n* fix self-review findings\n\n* revert accidental ws changes\n\n* fix attachment_clone\n\ntype & content_type are passed separately and content_type is cloned in\nsentry__attachments_add()\n\n* unit-testable sentry__path_unique() to back the \"-N.tar.gz\" claims\n\n* include <string>\n\n* ref: unique paths for byte attachments\n\n* add note about unique file names with crashpad\n\n* add missing null checks for screenshots\n\n* attachment_clone: add missing error handling\n\n* add note and missing test for buffer attachment comparison\n\n* Bump external/crashpad\n\n* Update external/crashpad\n\n* attachment_eq: clarify with a comment\n\n* document behavior regarding duplicate attachments\n\n* sentry__attachments_remove: replace attachment_eq with ptr cmp",
          "timestamp": "2025-06-24T16:27:14+02:00",
          "tree_id": "9e05ee000fa10a5e67a901a9fb21081b48123122",
          "url": "https://github.com/getsentry/sentry-native/commit/9dd7d6c61a1a3406fc613b4e2bad40baacc5453f"
        },
        "date": 1750775373629,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7059600000047794,
            "unit": "ms",
            "extra": "Min 0.683ms\nMax 0.745ms\nMean 0.707ms\nStdDev 0.024ms\nMedian 0.706ms\nCPU 0.706ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6959419999930105,
            "unit": "ms",
            "extra": "Min 0.686ms\nMax 0.724ms\nMean 0.702ms\nStdDev 0.016ms\nMedian 0.696ms\nCPU 0.702ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.815592999979799,
            "unit": "ms",
            "extra": "Min 2.788ms\nMax 2.858ms\nMean 2.822ms\nStdDev 0.029ms\nMedian 2.816ms\nCPU 1.490ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012073000050349947,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02219200001718491,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.031ms\nMean 0.024ms\nStdDev 0.004ms\nMedian 0.022ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7928589999769429,
            "unit": "ms",
            "extra": "Min 1.733ms\nMax 1.828ms\nMean 1.783ms\nStdDev 0.038ms\nMedian 1.793ms\nCPU 0.561ms"
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
          "id": "75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79",
          "message": "fix: xbox config typo (#1286)",
          "timestamp": "2025-06-24T20:48:09+02:00",
          "tree_id": "1b79f2f0327ba5e69ff5d101608f6752ebebb5b5",
          "url": "https://github.com/getsentry/sentry-native/commit/75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79"
        },
        "date": 1750791031743,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7256559999859746,
            "unit": "ms",
            "extra": "Min 0.708ms\nMax 0.769ms\nMean 0.734ms\nStdDev 0.023ms\nMedian 0.726ms\nCPU 0.734ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7114489999935358,
            "unit": "ms",
            "extra": "Min 0.705ms\nMax 0.801ms\nMean 0.729ms\nStdDev 0.040ms\nMedian 0.711ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9599030000042603,
            "unit": "ms",
            "extra": "Min 2.897ms\nMax 2.976ms\nMean 2.939ms\nStdDev 0.038ms\nMedian 2.960ms\nCPU 1.551ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012303000005431386,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022322000006624876,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.812731000001122,
            "unit": "ms",
            "extra": "Min 1.795ms\nMax 1.885ms\nMean 1.830ms\nStdDev 0.038ms\nMedian 1.813ms\nCPU 0.575ms"
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
          "id": "7646cac9c760c6734741d96320ec1e084b1e00f3",
          "message": "feat: add `sentry_attachment_set_filename()` (#1285)\n\n* feat: add `sentry_attachment_set_filename()`\n\n* add missing null check\n\n* crashpad: adapt ensure_unique_path",
          "timestamp": "2025-06-25T09:10:39+02:00",
          "tree_id": "6662413ce89811f88fc9b2b01f77313e006a6d87",
          "url": "https://github.com/getsentry/sentry-native/commit/7646cac9c760c6734741d96320ec1e084b1e00f3"
        },
        "date": 1750835569687,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7058230000041021,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.812ms\nMean 0.725ms\nStdDev 0.050ms\nMedian 0.706ms\nCPU 0.713ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7065339999883236,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.728ms\nMean 0.708ms\nStdDev 0.016ms\nMedian 0.707ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.969621000005418,
            "unit": "ms",
            "extra": "Min 2.886ms\nMax 2.974ms\nMean 2.942ms\nStdDev 0.040ms\nMedian 2.970ms\nCPU 1.497ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012292999997498555,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022651999984191207,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7513479999990977,
            "unit": "ms",
            "extra": "Min 1.732ms\nMax 1.771ms\nMean 1.752ms\nStdDev 0.015ms\nMedian 1.751ms\nCPU 0.532ms"
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
          "id": "28fb3edd0ef3638059ab86d4f734f6cc5f0c9652",
          "message": "meta: identify Xbox as a separate SDK name (#1287)\n\n* Update CHANGELOG.md\n* move static sdk identification + versioning below platform defs in order to reuse the platform defs rather than external ones.",
          "timestamp": "2025-06-25T10:52:07+02:00",
          "tree_id": "30ce0a45de61b725825d2c3bbd76a94c01d5a8bd",
          "url": "https://github.com/getsentry/sentry-native/commit/28fb3edd0ef3638059ab86d4f734f6cc5f0c9652"
        },
        "date": 1750841658254,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7127999999738677,
            "unit": "ms",
            "extra": "Min 0.689ms\nMax 0.732ms\nMean 0.711ms\nStdDev 0.016ms\nMedian 0.713ms\nCPU 0.711ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7233489999975973,
            "unit": "ms",
            "extra": "Min 0.684ms\nMax 0.735ms\nMean 0.719ms\nStdDev 0.020ms\nMedian 0.723ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.85450699999501,
            "unit": "ms",
            "extra": "Min 2.810ms\nMax 3.012ms\nMean 2.881ms\nStdDev 0.081ms\nMedian 2.855ms\nCPU 1.514ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012293000025920264,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022802999978921434,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8334399999844209,
            "unit": "ms",
            "extra": "Min 1.752ms\nMax 1.931ms\nMean 1.829ms\nStdDev 0.068ms\nMedian 1.833ms\nCPU 0.565ms"
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
          "id": "e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112",
          "message": "chore: enable PS stack unwinding and module resolution (#1282)\n\n* chore: enable PS stack unwinding and module resolution\n\n* symbolize stacktrace automatically on PS",
          "timestamp": "2025-06-25T11:09:57+02:00",
          "tree_id": "f660999902c870f1ee163adc984d2245c84d258a",
          "url": "https://github.com/getsentry/sentry-native/commit/e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112"
        },
        "date": 1750842739186,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7937620000006973,
            "unit": "ms",
            "extra": "Min 0.785ms\nMax 0.835ms\nMean 0.804ms\nStdDev 0.022ms\nMedian 0.794ms\nCPU 0.802ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7263560000012603,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.751ms\nMean 0.726ms\nStdDev 0.024ms\nMedian 0.726ms\nCPU 0.726ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.214049000007435,
            "unit": "ms",
            "extra": "Min 3.031ms\nMax 3.628ms\nMean 3.304ms\nStdDev 0.247ms\nMedian 3.214ms\nCPU 1.635ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012464000008094445,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02243200000862089,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8661720000068271,
            "unit": "ms",
            "extra": "Min 1.860ms\nMax 1.917ms\nMean 1.877ms\nStdDev 0.023ms\nMedian 1.866ms\nCPU 0.597ms"
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
          "id": "3f7bbd6193bda6814a7258da31971f21bf61da26",
          "message": "fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined (#1279)\n\n* fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined\n\n* run unit tests with custom path prefix\n\n* linter issue\n\n* fix cmake.py\n\n* Update tests/unit/sentry_testsupport.h",
          "timestamp": "2025-06-25T11:10:59+02:00",
          "tree_id": "be3a9dccafda19c04122b5283e8e33c4e4f818a2",
          "url": "https://github.com/getsentry/sentry-native/commit/3f7bbd6193bda6814a7258da31971f21bf61da26"
        },
        "date": 1750842790750,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7117220000054658,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.747ms\nMean 0.711ms\nStdDev 0.023ms\nMedian 0.712ms\nCPU 0.711ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7212300000105643,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.728ms\nMean 0.716ms\nStdDev 0.015ms\nMedian 0.721ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.866212000071755,
            "unit": "ms",
            "extra": "Min 2.799ms\nMax 3.026ms\nMean 2.884ms\nStdDev 0.094ms\nMedian 2.866ms\nCPU 1.514ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012351999998827523,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022252000007938477,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7961080000077345,
            "unit": "ms",
            "extra": "Min 1.778ms\nMax 1.864ms\nMean 1.804ms\nStdDev 0.034ms\nMedian 1.796ms\nCPU 0.573ms"
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
          "id": "6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc",
          "message": "Merge branch 'release/0.9.1'",
          "timestamp": "2025-06-25T11:07:11Z",
          "tree_id": "51b4af13587545183a4c80ccce4bad6490ba3728",
          "url": "https://github.com/getsentry/sentry-native/commit/6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc"
        },
        "date": 1750849835537,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7076299999937419,
            "unit": "ms",
            "extra": "Min 0.701ms\nMax 0.734ms\nMean 0.711ms\nStdDev 0.013ms\nMedian 0.708ms\nCPU 0.710ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7084500000189564,
            "unit": "ms",
            "extra": "Min 0.689ms\nMax 0.723ms\nMean 0.707ms\nStdDev 0.012ms\nMedian 0.708ms\nCPU 0.707ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8766749999817876,
            "unit": "ms",
            "extra": "Min 2.829ms\nMax 3.151ms\nMean 2.926ms\nStdDev 0.131ms\nMedian 2.877ms\nCPU 1.503ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01226300003054348,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02251099999739381,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8714189999968767,
            "unit": "ms",
            "extra": "Min 1.823ms\nMax 1.889ms\nMean 1.866ms\nStdDev 0.027ms\nMedian 1.871ms\nCPU 0.567ms"
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
          "id": "d3eef89221f127831ea58ac06797c00c96a1d63c",
          "message": "chore: add clangd .cache to .gitignore (#1291)",
          "timestamp": "2025-06-30T14:43:53+02:00",
          "tree_id": "92e0c2dbe387b8c88d769a05c4b5e6c6f248dff7",
          "url": "https://github.com/getsentry/sentry-native/commit/d3eef89221f127831ea58ac06797c00c96a1d63c"
        },
        "date": 1751287689044,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.765648000026431,
            "unit": "ms",
            "extra": "Min 0.749ms\nMax 0.877ms\nMean 0.787ms\nStdDev 0.052ms\nMedian 0.766ms\nCPU 0.764ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7243609999818545,
            "unit": "ms",
            "extra": "Min 0.694ms\nMax 0.730ms\nMean 0.716ms\nStdDev 0.015ms\nMedian 0.724ms\nCPU 0.716ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.97220599998127,
            "unit": "ms",
            "extra": "Min 2.891ms\nMax 3.555ms\nMean 3.070ms\nStdDev 0.275ms\nMedian 2.972ms\nCPU 1.526ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01215199995385774,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022561999969639146,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.109ms\nMean 0.040ms\nStdDev 0.038ms\nMedian 0.023ms\nCPU 0.030ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7729929999745764,
            "unit": "ms",
            "extra": "Min 1.747ms\nMax 1.804ms\nMean 1.774ms\nStdDev 0.023ms\nMedian 1.773ms\nCPU 0.559ms"
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
          "id": "6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4",
          "message": "ci: update kcov (#1292)\n\n* pin kcov at 8afe9f29c58ef575877664c7ba209328233b70cc",
          "timestamp": "2025-06-30T16:23:35+02:00",
          "tree_id": "fe98eff320080abd8001df117dedf875aac1d839",
          "url": "https://github.com/getsentry/sentry-native/commit/6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4"
        },
        "date": 1751293545844,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7244869999993853,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.745ms\nMean 0.722ms\nStdDev 0.020ms\nMedian 0.724ms\nCPU 0.722ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7228740000044809,
            "unit": "ms",
            "extra": "Min 0.690ms\nMax 0.737ms\nMean 0.718ms\nStdDev 0.018ms\nMedian 0.723ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.81952000000274,
            "unit": "ms",
            "extra": "Min 2.811ms\nMax 2.955ms\nMean 2.846ms\nStdDev 0.062ms\nMedian 2.820ms\nCPU 1.514ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012182000006077942,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.023083000002088738,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.044ms\nMean 0.027ms\nStdDev 0.010ms\nMedian 0.023ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7824170000153572,
            "unit": "ms",
            "extra": "Min 1.742ms\nMax 1.919ms\nMean 1.820ms\nStdDev 0.076ms\nMedian 1.782ms\nCPU 0.574ms"
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
          "id": "b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a",
          "message": "feat: add `sentry_clear_attachments()` (#1290)\n\n* feat: add `sentry_clear_attachments()`\n\n* crashpad: clear attachments in one go\n\n* Bump external/crashpad\n\n* Update CHANGELOG.md\n\n* revise remove_buffer_file()\n\n* add integration test\n\n* revert clear_attachments_func\n\nan integration test revealed that it was too effective clearing even\nbreadcrumb attachments\n\n* swap lock order",
          "timestamp": "2025-06-30T20:49:54+02:00",
          "tree_id": "6e201fe7cb9ba9838bcaf761a2f37e56a20695de",
          "url": "https://github.com/getsentry/sentry-native/commit/b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a"
        },
        "date": 1751309554539,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7024550000096497,
            "unit": "ms",
            "extra": "Min 0.697ms\nMax 0.745ms\nMean 0.714ms\nStdDev 0.021ms\nMedian 0.702ms\nCPU 0.710ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7042769999969778,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.827ms\nMean 0.725ms\nStdDev 0.057ms\nMedian 0.704ms\nCPU 0.708ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.885062000018479,
            "unit": "ms",
            "extra": "Min 2.816ms\nMax 2.992ms\nMean 2.892ms\nStdDev 0.064ms\nMedian 2.885ms\nCPU 1.523ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012263000002121771,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022292000011248092,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.120ms\nMean 0.042ms\nStdDev 0.044ms\nMedian 0.022ms\nCPU 0.030ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8124449999845638,
            "unit": "ms",
            "extra": "Min 1.755ms\nMax 1.870ms\nMean 1.805ms\nStdDev 0.045ms\nMedian 1.812ms\nCPU 0.559ms"
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
          "id": "3d9c118b7314f76c2350dfdd819e39feda033bf3",
          "message": "feat: add `sentry_regenerate_trace()` (#1293)\n\n* add generate_trace() + test\n\n* changelog\n\n* fix\n\n* fix memleak\n\n* remove const to allow sentry_free\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-01T11:14:09+02:00",
          "tree_id": "abfab1993ac4ca8f50dc33019aa6de9b251dd9ae",
          "url": "https://github.com/getsentry/sentry-native/commit/3d9c118b7314f76c2350dfdd819e39feda033bf3"
        },
        "date": 1751361385709,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.8065560000432015,
            "unit": "ms",
            "extra": "Min 0.743ms\nMax 0.885ms\nMean 0.805ms\nStdDev 0.054ms\nMedian 0.807ms\nCPU 0.791ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7530960000394771,
            "unit": "ms",
            "extra": "Min 0.722ms\nMax 0.778ms\nMean 0.752ms\nStdDev 0.022ms\nMedian 0.753ms\nCPU 0.752ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8710910000881995,
            "unit": "ms",
            "extra": "Min 2.809ms\nMax 2.970ms\nMean 2.880ms\nStdDev 0.071ms\nMedian 2.871ms\nCPU 1.515ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01223299989305815,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02211100002114108,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8267289999585046,
            "unit": "ms",
            "extra": "Min 1.769ms\nMax 1.845ms\nMean 1.814ms\nStdDev 0.035ms\nMedian 1.827ms\nCPU 0.559ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bruno@brunogarcia.com",
            "name": "Bruno Garcia",
            "username": "bruno-garcia"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e0b3f8bba161a732463de2f32e00e89d154f7363",
          "message": "compiles on xbox one (#1294)\n\n* feat: xbox one support\n\n* define\n\n* wip\n\n* multiline command\n\n* it builds\n\n* gxdk_base_toolchain.cmake\n\n* update external/breakpad\n\n* fix'n'tweak xbox defines\n\n* Revert \"gxdk_base_toolchain.cmake\"\n\nThis reverts commit 18541aa8e3cada297d5212932dc1ec898ed5d5e6.\n\n* gxdk_toolchain.cmake - take 2\n\nhttps://github.com/microsoft/Xbox-GDK-Samples/blob/aa45b831e7a71160a69a7d13e9d74844dc6aa210/Samples/Tools/CMakeGDKExample/gxdk_toolchain.cmake\n\n* fix formatting\n\n* toolchain md\n\n* changelog\n\n* Update external/breakpad\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-03T16:28:39-04:00",
          "tree_id": "53f3f9cfc8842ce0d121144d90fddafb2b803f5c",
          "url": "https://github.com/getsentry/sentry-native/commit/e0b3f8bba161a732463de2f32e00e89d154f7363"
        },
        "date": 1751574653544,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.6976479999991625,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.721ms\nMean 0.701ms\nStdDev 0.013ms\nMedian 0.698ms\nCPU 0.700ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.6884109999987231,
            "unit": "ms",
            "extra": "Min 0.687ms\nMax 0.703ms\nMean 0.694ms\nStdDev 0.008ms\nMedian 0.688ms\nCPU 0.693ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8273270000056527,
            "unit": "ms",
            "extra": "Min 2.792ms\nMax 2.956ms\nMean 2.846ms\nStdDev 0.065ms\nMedian 2.827ms\nCPU 1.486ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012352999988252122,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02168099999266815,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.74927599999819,
            "unit": "ms",
            "extra": "Min 1.714ms\nMax 1.823ms\nMean 1.756ms\nStdDev 0.040ms\nMedian 1.749ms\nCPU 0.545ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "rom4ek93@gmail.com",
            "name": "Roman Zavarnitsyn",
            "username": "romtsn"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "98b1bd883824ad6a63c2e6dcc2f07be407c73215",
          "message": "ci(publishing): Use shim ossrh-staging-api server to publish to the new central repo  (#1299)\n\n* Update settings.xml\n\n* Update .craft.yml",
          "timestamp": "2025-07-07T10:21:01+02:00",
          "tree_id": "f6021f5cba3e334cd66083b22d42914810c758ac",
          "url": "https://github.com/getsentry/sentry-native/commit/98b1bd883824ad6a63c2e6dcc2f07be407c73215"
        },
        "date": 1751876607804,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7218990000126269,
            "unit": "ms",
            "extra": "Min 0.694ms\nMax 0.794ms\nMean 0.736ms\nStdDev 0.042ms\nMedian 0.722ms\nCPU 0.717ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7222380000087014,
            "unit": "ms",
            "extra": "Min 0.710ms\nMax 0.755ms\nMean 0.727ms\nStdDev 0.018ms\nMedian 0.722ms\nCPU 0.727ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.957680999998047,
            "unit": "ms",
            "extra": "Min 2.881ms\nMax 3.053ms\nMean 2.954ms\nStdDev 0.073ms\nMedian 2.958ms\nCPU 1.527ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01225299999418894,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02276300000403353,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.047ms\nMean 0.027ms\nStdDev 0.011ms\nMedian 0.023ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7804850000118222,
            "unit": "ms",
            "extra": "Min 1.752ms\nMax 1.850ms\nMean 1.786ms\nStdDev 0.039ms\nMedian 1.780ms\nCPU 0.565ms"
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
          "id": "092202901f9b6fc16e94b7ce4ba2ac2b0afaee41",
          "message": "chore: update xbox toolchain and disable FS-dependent tests (#1302)\n\n* disable tests that require FS access\n\n* disable modulefinder test\n\n* chore: update xbox toolchains\n\n* fixup\n\n* try to reenable modulefinder\n\n* chore: changelog",
          "timestamp": "2025-07-08T18:10:43+02:00",
          "tree_id": "d5a97fdb483155e76f97d6a071eb0df625af382b",
          "url": "https://github.com/getsentry/sentry-native/commit/092202901f9b6fc16e94b7ce4ba2ac2b0afaee41"
        },
        "date": 1751991205664,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7244850000063252,
            "unit": "ms",
            "extra": "Min 0.697ms\nMax 0.732ms\nMean 0.719ms\nStdDev 0.014ms\nMedian 0.724ms\nCPU 0.719ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7154979999768329,
            "unit": "ms",
            "extra": "Min 0.691ms\nMax 0.735ms\nMean 0.713ms\nStdDev 0.017ms\nMedian 0.715ms\nCPU 0.713ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.2067379999602963,
            "unit": "ms",
            "extra": "Min 3.103ms\nMax 3.566ms\nMean 3.265ms\nStdDev 0.176ms\nMedian 3.207ms\nCPU 1.652ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01358500003334484,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.015ms\nMean 0.014ms\nStdDev 0.001ms\nMedian 0.014ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022832000013295328,
            "unit": "ms",
            "extra": "Min 0.023ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8610429999625921,
            "unit": "ms",
            "extra": "Min 1.812ms\nMax 1.920ms\nMean 1.863ms\nStdDev 0.040ms\nMedian 1.861ms\nCPU 0.591ms"
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
          "id": "9ed9a70d71d98a1f38f8194334c9d1a40bcaed85",
          "message": "meta: add SENTRY_DEPRECATED(msg) macro (#1308)\n\n* meta: add SENTRY_DEPRECATED(msg) macro\n\n* fix deprecation warnings in tests\n\n* Update CHANGELOG.md\n\n* add version checks to be safe\n\n* try to make msvc happy\n\n* add SENTRY_TEST_DEPRECATED\n\n* Revert \"fix deprecation warnings in tests\"\n\nThis reverts commit 0245434bd1a37e3057111e6bb0272c23c2157963.\n\n* Use SENTRY_TEST_DEPRECATED\n\n* fix SENTRY_TEST_DEPRECATED formatting",
          "timestamp": "2025-07-10T18:15:12+02:00",
          "tree_id": "411d1040913b5a1fd7d66d55cd2fd3756c37ce12",
          "url": "https://github.com/getsentry/sentry-native/commit/9ed9a70d71d98a1f38f8194334c9d1a40bcaed85"
        },
        "date": 1752164257087,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7074179999904118,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.744ms\nMean 0.713ms\nStdDev 0.023ms\nMedian 0.707ms\nCPU 0.711ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7158549999957131,
            "unit": "ms",
            "extra": "Min 0.701ms\nMax 0.723ms\nMean 0.715ms\nStdDev 0.009ms\nMedian 0.716ms\nCPU 0.714ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.886969000002182,
            "unit": "ms",
            "extra": "Min 2.830ms\nMax 2.936ms\nMean 2.883ms\nStdDev 0.041ms\nMedian 2.887ms\nCPU 1.520ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012263000002121771,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022472000011930504,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8118319999871346,
            "unit": "ms",
            "extra": "Min 1.770ms\nMax 1.884ms\nMean 1.823ms\nStdDev 0.043ms\nMedian 1.812ms\nCPU 0.589ms"
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
          "id": "bffc57deb53808b26e11ef2683c05340195f8b0d",
          "message": "feat: implement the new User Feedback API (#1304)\n\n* feat: implement the new User Feedback API\n\nhttps://develop.sentry.dev/sdk/data-model/envelope-items/#user-feedback\n\n* Fix heap-use-after-free\n\n* Update CHANGELOG.md\n\n* remove unused line3_end\n\n* what if we reused the old API?\n\n* use internal id to fix event association in Sentry Web UI\n\n* restore integration test for legacy user report\n\n* convert a deprecated \"user report\" to a new \"user feedback\"\n\n* conversion: note in docs & log info message\n\n* let sentry__envelope_add_user_feedback take ownership\n\n* revert back to option 1\n\n* deprecate sentry_value_new_user_feedback() & sentry_capture_user_feedback()\n\n* fix remaining deprecation warnings\n\n* msvc",
          "timestamp": "2025-07-11T12:28:24+02:00",
          "tree_id": "09bfefa430d3e1795ffea5c12ce54d3a1bc21273",
          "url": "https://github.com/getsentry/sentry-native/commit/bffc57deb53808b26e11ef2683c05340195f8b0d"
        },
        "date": 1752229851997,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7204630000217094,
            "unit": "ms",
            "extra": "Min 0.688ms\nMax 0.744ms\nMean 0.718ms\nStdDev 0.023ms\nMedian 0.720ms\nCPU 0.717ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7285080000087873,
            "unit": "ms",
            "extra": "Min 0.700ms\nMax 0.731ms\nMean 0.723ms\nStdDev 0.013ms\nMedian 0.729ms\nCPU 0.722ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.97037400000022,
            "unit": "ms",
            "extra": "Min 2.857ms\nMax 2.985ms\nMean 2.950ms\nStdDev 0.052ms\nMedian 2.970ms\nCPU 1.527ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012363999985609553,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02185099998541773,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.028ms\nMean 0.023ms\nStdDev 0.003ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.845507999973961,
            "unit": "ms",
            "extra": "Min 1.809ms\nMax 1.884ms\nMean 1.845ms\nStdDev 0.028ms\nMedian 1.846ms\nCPU 0.594ms"
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
          "id": "fef7868244cd34c1a6d8650d3c594b0bf2a47314",
          "message": "test: migrate from deprecated `sentry_new_function_transport` (#1310)",
          "timestamp": "2025-07-11T15:15:44+02:00",
          "tree_id": "4ac1f0a0ba99d764784cf93c3294007a95774d7f",
          "url": "https://github.com/getsentry/sentry-native/commit/fef7868244cd34c1a6d8650d3c594b0bf2a47314"
        },
        "date": 1752239878945,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.707983999973294,
            "unit": "ms",
            "extra": "Min 0.684ms\nMax 0.718ms\nMean 0.706ms\nStdDev 0.013ms\nMedian 0.708ms\nCPU 0.706ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.718744999971932,
            "unit": "ms",
            "extra": "Min 0.701ms\nMax 0.733ms\nMean 0.719ms\nStdDev 0.013ms\nMedian 0.719ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8200450000213095,
            "unit": "ms",
            "extra": "Min 2.772ms\nMax 2.885ms\nMean 2.826ms\nStdDev 0.048ms\nMedian 2.820ms\nCPU 1.488ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01232299996445363,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022080999997342587,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7705840000417084,
            "unit": "ms",
            "extra": "Min 1.749ms\nMax 1.836ms\nMean 1.779ms\nStdDev 0.033ms\nMedian 1.771ms\nCPU 0.558ms"
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
          "id": "cbaaf1e9590781327ff9eda35b01fefcddd669c0",
          "message": "feat: add dynamic sampling context (#1254)\n\n* initial DSC implementation\n\n* add `sample_rand`\n\n* fix `sample_rand` generation\n\n* fix the fix + add test\n\n* update org_id expected size\n\n* add test + changelog + sample_rand for traces_sampler\n\n* add test + changelog + sample_rand for traces_sampler + dsc for events\n\n* add org_id tests\n\n* fix CHANGELOG.md\n\n* cleanup\n\n* post-merge fix\n\n* post-merge fix v2\n\n* split up test\n\n* fix CHANGELOG.md\n\n* org_id handling update\n\n* asan overflow fix\n\n* add warnings for missing DSC fields\n\n* swap `sentry_set_trace` for `sentry_regenerate_trace`\n\n* add clarifying note\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-15T11:28:05+02:00",
          "tree_id": "5c65231fce6629c7190a21ad5faca8a1575b5042",
          "url": "https://github.com/getsentry/sentry-native/commit/cbaaf1e9590781327ff9eda35b01fefcddd669c0"
        },
        "date": 1752571828617,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7482989999942902,
            "unit": "ms",
            "extra": "Min 0.729ms\nMax 0.774ms\nMean 0.747ms\nStdDev 0.017ms\nMedian 0.748ms\nCPU 0.747ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.727581000006694,
            "unit": "ms",
            "extra": "Min 0.708ms\nMax 0.751ms\nMean 0.727ms\nStdDev 0.016ms\nMedian 0.728ms\nCPU 0.727ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.18006200001264,
            "unit": "ms",
            "extra": "Min 3.131ms\nMax 3.202ms\nMean 3.175ms\nStdDev 0.026ms\nMedian 3.180ms\nCPU 1.650ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012464000008094445,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.023122999976976644,
            "unit": "ms",
            "extra": "Min 0.023ms\nMax 0.028ms\nMean 0.024ms\nStdDev 0.002ms\nMedian 0.023ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.9297679999965567,
            "unit": "ms",
            "extra": "Min 1.915ms\nMax 2.001ms\nMean 1.949ms\nStdDev 0.037ms\nMedian 1.930ms\nCPU 0.616ms"
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
          "id": "68b918dfd331f218904739b90fe68dd54a1d93ba",
          "message": "test: fix SENTRY_TEST(path_directory) to clean up after itself (#1314)",
          "timestamp": "2025-07-15T13:02:21+02:00",
          "tree_id": "911f165201c3be7020da3d8635688367e6290b8e",
          "url": "https://github.com/getsentry/sentry-native/commit/68b918dfd331f218904739b90fe68dd54a1d93ba"
        },
        "date": 1752577485015,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7334890000123551,
            "unit": "ms",
            "extra": "Min 0.717ms\nMax 0.762ms\nMean 0.736ms\nStdDev 0.016ms\nMedian 0.733ms\nCPU 0.736ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7698560000051202,
            "unit": "ms",
            "extra": "Min 0.750ms\nMax 0.835ms\nMean 0.787ms\nStdDev 0.037ms\nMedian 0.770ms\nCPU 0.787ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.108730999997533,
            "unit": "ms",
            "extra": "Min 3.006ms\nMax 3.152ms\nMean 3.093ms\nStdDev 0.054ms\nMedian 3.109ms\nCPU 1.600ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01257299999224415,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.013ms\nStdDev 0.000ms\nMedian 0.013ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022803000007343144,
            "unit": "ms",
            "extra": "Min 0.023ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8395669999904385,
            "unit": "ms",
            "extra": "Min 1.799ms\nMax 1.901ms\nMean 1.839ms\nStdDev 0.041ms\nMedian 1.840ms\nCPU 0.574ms"
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
          "id": "2b92ca838366ddc76a464e57795c7aaea2265d34",
          "message": "fix(xbox): propagate GDK version to MSBuild (#1312)\n\n* fix: propagate GDK version to MSBuild\n\n* Update CHANGELOG.md",
          "timestamp": "2025-07-15T16:38:52+02:00",
          "tree_id": "f20bb7a137cc3952ac937135be5bd26084682f7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2b92ca838366ddc76a464e57795c7aaea2265d34"
        },
        "date": 1752590522531,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7319580000739734,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.772ms\nMean 0.731ms\nStdDev 0.027ms\nMedian 0.732ms\nCPU 0.730ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7201260000329057,
            "unit": "ms",
            "extra": "Min 0.696ms\nMax 0.726ms\nMean 0.715ms\nStdDev 0.012ms\nMedian 0.720ms\nCPU 0.714ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9641300000093906,
            "unit": "ms",
            "extra": "Min 2.867ms\nMax 3.545ms\nMean 3.104ms\nStdDev 0.287ms\nMedian 2.964ms\nCPU 1.506ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01232299996445363,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02228200003173697,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7759690000502815,
            "unit": "ms",
            "extra": "Min 1.770ms\nMax 1.833ms\nMean 1.797ms\nStdDev 0.032ms\nMedian 1.776ms\nCPU 0.576ms"
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
          "id": "70f38a25655323f2e1e336a8771725c335c41319",
          "message": "ci: enable tsan + fix: rework breadcrumb ringbuffer (#1309)\n\n* adapt python test runner\n\n* fix lock-order inversion in `sentry_start_session()`\n\nThis will typically not hit because the inversion is relevant with the lock acquisition in `sentry_init()` and these two are typically not called from two different threads at the same time.\n\nHowever, consistent lock-order is such a basic sanity step and will guard us from future functions that also require scope- and options-locks.\n\n* properly synchronize `executed_after_shutdown` in `SENTRY_TEST(task_queue)`\n\n* move pre-init in sentry_init outside the options lock\n\n* move set_context outside the scope lock in set_trace so we can prevent unnecessary scope lock recursion\n\n* fix early exit in sentry__scope_get_span_or_transaction (only relevant for tests)\n\n* fix concurrent access to TEST_CHECK macro, which updates an unsynced global in the test-framework\n\n* minor format\n\n* extract the ringbuffer into a separate module\n\nthis allows us to keep state close and remove the need to retrieve max_breadcrumbs from the options after sentry_init(). Which in turn removes the need for nested locks.\n\n* add \"lock handling\" to the contributor docs\n\n* synchronize access to acutest state in the concurrency unit tests\n\n* limit SENTRY_WITH_SCOPE in attachment tests to the lines that actually need the scope\n\n* introduce tsan configs into the matrix\n\n* remove ringbuffer get_len definition\n\n* add tsan.supp + support in test runner\n\n* disable tsan in integration tests that involve DEADLYSIGNALS\n\n* add GCC based TSAN tests as a toolchain cross-check\n\n* clean up\n\n* revert Werror for GCC due to crashpad (not solving this here)\n\n* increase TSAN verbosity for the internal check that fails\n\n* disable all crashpad runs for tsan since it fails an internal check when forking the handler\n\n* format\n\n* re-enable all crashing tests in the http/stdout integration tests\n\nI falsely assumed an issue in the signal handler itself because the internal check triggered by crashpad_handler forking coincided with DEADLYSIGNAL.\n\nTurns out DEADLYSIGNAL is handled in TSAN like in ASAN, i.e. non-fatal (puh).\n\nThe culprit for crashpad is the fork during initialization, which breaks with TSAN invariants.\n\n* move ringbuffer tests in separate tu\n\n* missed cmake update\n\n* get rid of another unnecessary lock nesting\n\n* clone the DSC when prepping for the envelope header\n\n* clone the DSC when prepping for the envelope header (also in event path)\n\n* dramatically limit scope lock when prepping envelopes.\n\nAlso ensure that the cloned DSC is freed if we don't add it as a header.\n\n* remove unused imports\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-16T17:21:01+02:00",
          "tree_id": "2861324adb9817ab119e8c2571dda42436827416",
          "url": "https://github.com/getsentry/sentry-native/commit/70f38a25655323f2e1e336a8771725c335c41319"
        },
        "date": 1752679397791,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7171340000127202,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.740ms\nMean 0.720ms\nStdDev 0.016ms\nMedian 0.717ms\nCPU 0.720ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7228249999968739,
            "unit": "ms",
            "extra": "Min 0.714ms\nMax 0.814ms\nMean 0.739ms\nStdDev 0.042ms\nMedian 0.723ms\nCPU 0.726ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9558749999978318,
            "unit": "ms",
            "extra": "Min 2.847ms\nMax 2.961ms\nMean 2.919ms\nStdDev 0.053ms\nMedian 2.956ms\nCPU 1.515ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012142999992192927,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022220999994715385,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.77151100001538,
            "unit": "ms",
            "extra": "Min 1.720ms\nMax 1.822ms\nMean 1.771ms\nStdDev 0.037ms\nMedian 1.772ms\nCPU 0.542ms"
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
          "id": "c3aa571d4eceffcaf4987cde769c92f2bc099753",
          "message": "feat: add `sentry__path_get_mtime()` (#1317)\n\n* feat: add `sentry__path_get_mtime()`\n\n* Apply suggestions from code review\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T16:03:41+02:00",
          "tree_id": "3baeb34b123904e7fdce41b8585a07d9d6e6d617",
          "url": "https://github.com/getsentry/sentry-native/commit/c3aa571d4eceffcaf4987cde769c92f2bc099753"
        },
        "date": 1753452380338,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7285300000603456,
            "unit": "ms",
            "extra": "Min 0.696ms\nMax 0.732ms\nMean 0.722ms\nStdDev 0.015ms\nMedian 0.729ms\nCPU 0.721ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7171769999558819,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.736ms\nMean 0.717ms\nStdDev 0.017ms\nMedian 0.717ms\nCPU 0.717ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8770669999858,
            "unit": "ms",
            "extra": "Min 2.790ms\nMax 2.906ms\nMean 2.860ms\nStdDev 0.050ms\nMedian 2.877ms\nCPU 1.505ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012233000006744987,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02242199991542293,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7819740000959428,
            "unit": "ms",
            "extra": "Min 1.741ms\nMax 1.828ms\nMean 1.785ms\nStdDev 0.033ms\nMedian 1.782ms\nCPU 0.552ms"
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
          "id": "205831fda2ece8c784259f7abac6d297a65474a6",
          "message": "ref: crashpad: pre-generate crash event ID (#1319)\n\n* ref: crashpad: generate event ID on the client side\n\n* Update CHANGELOG.md\n\n* Update src/sentry_value.c\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* fixup\n\n* Update src/sentry_value.h\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T17:30:53+02:00",
          "tree_id": "2ae22807fa39861d2995772f90de749f8f52b9b0",
          "url": "https://github.com/getsentry/sentry-native/commit/205831fda2ece8c784259f7abac6d297a65474a6"
        },
        "date": 1753457663708,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7129520000148659,
            "unit": "ms",
            "extra": "Min 0.695ms\nMax 0.819ms\nMean 0.731ms\nStdDev 0.050ms\nMedian 0.713ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7218669999815575,
            "unit": "ms",
            "extra": "Min 0.698ms\nMax 0.746ms\nMean 0.722ms\nStdDev 0.019ms\nMedian 0.722ms\nCPU 0.721ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8835449999746743,
            "unit": "ms",
            "extra": "Min 2.825ms\nMax 3.007ms\nMean 2.905ms\nStdDev 0.080ms\nMedian 2.884ms\nCPU 1.514ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012142999992192927,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022322000006624876,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7731949999983954,
            "unit": "ms",
            "extra": "Min 1.726ms\nMax 1.805ms\nMean 1.769ms\nStdDev 0.028ms\nMedian 1.773ms\nCPU 0.556ms"
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
          "id": "1939e5d2b1636fce6320208a451b51a41aadcc47",
          "message": "test: suppress valgrind for `SENTRY_TEST(task_queue)` (#1323)",
          "timestamp": "2025-07-26T08:43:59-04:00",
          "tree_id": "f780ac08a01bbf8976d4039c54192f7f6d697977",
          "url": "https://github.com/getsentry/sentry-native/commit/1939e5d2b1636fce6320208a451b51a41aadcc47"
        },
        "date": 1753534022337,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7621630000045343,
            "unit": "ms",
            "extra": "Min 0.749ms\nMax 0.833ms\nMean 0.774ms\nStdDev 0.034ms\nMedian 0.762ms\nCPU 0.759ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7573439999930542,
            "unit": "ms",
            "extra": "Min 0.734ms\nMax 0.899ms\nMean 0.778ms\nStdDev 0.069ms\nMedian 0.757ms\nCPU 0.756ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9681460000006155,
            "unit": "ms",
            "extra": "Min 2.910ms\nMax 3.159ms\nMean 3.004ms\nStdDev 0.097ms\nMedian 2.968ms\nCPU 1.555ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012373000004117785,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022582000013926518,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7881060000206617,
            "unit": "ms",
            "extra": "Min 1.787ms\nMax 1.877ms\nMean 1.815ms\nStdDev 0.041ms\nMedian 1.788ms\nCPU 0.556ms"
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
          "id": "2ac959eff4f76935fcdd4c81dd1cca1056c01f77",
          "message": "feat: read and capture envelopes (#1320)\n\n* feat: raw envelope headers\n\n* Update CHANGELOG.md\n\n* move parsing to sentry_envelope_read_from_file()\n\n* cast\n\n* feat: parse whole envelope\n\n* Fix ClangCL warnings\n\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(765,53): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(786,62): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(803,61): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\n\n* revert unnecessary changes\n\n* Wide-string variant for Windows\n\n* Improve tests\n\n* size_t payload_len\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* sentry_envelope_deserialize + tests\n\n* validate headers\n\n* don't advance ptr past the buffer even if it's never used\n\nmight make cursor satisfied?\n\n* sentry_capture_envelope: add extra null check\n\n* sentry_envelope_get_header_n: add null check\n\n* prevent overflow\n\n* add TODO comments for sentry_value_as_uint64 + SIZE_MAX\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>",
          "timestamp": "2025-07-29T07:54:02+02:00",
          "tree_id": "358171b00995b08d09eb24f60a2f2080cb3bbcb8",
          "url": "https://github.com/getsentry/sentry-native/commit/2ac959eff4f76935fcdd4c81dd1cca1056c01f77"
        },
        "date": 1753768582999,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7568090000091843,
            "unit": "ms",
            "extra": "Min 0.737ms\nMax 0.780ms\nMean 0.757ms\nStdDev 0.017ms\nMedian 0.757ms\nCPU 0.755ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7341459999992139,
            "unit": "ms",
            "extra": "Min 0.693ms\nMax 0.758ms\nMean 0.730ms\nStdDev 0.025ms\nMedian 0.734ms\nCPU 0.730ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9879660000062813,
            "unit": "ms",
            "extra": "Min 2.936ms\nMax 3.040ms\nMean 2.985ms\nStdDev 0.045ms\nMedian 2.988ms\nCPU 1.546ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012193000003435372,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.023374000022613473,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.036ms\nMean 0.026ms\nStdDev 0.006ms\nMedian 0.023ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8253469999933714,
            "unit": "ms",
            "extra": "Min 1.760ms\nMax 1.866ms\nMean 1.810ms\nStdDev 0.044ms\nMedian 1.825ms\nCPU 0.567ms"
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
          "id": "93d69f4172c530d8e4ac581df2e80d97e24c7398",
          "message": "feat: add `sentry__process_spawn()` (#1318)\n\n* feat: add `sentry__process_spawn()`\n\n* stabilize SENTRY_TEST(process_spawn)\n\n* add null checks for executable->path\n\n* test: drop sentry__process_spawn return value check\n\njust to see if the CI/Valgrind is happy with the rest\n\n* sentry__process_spawn: remove falsy return value\n\nas claude put it\n\n> This is inherent to fully detached processes - you can't reliably detect launch success without IPC\n\n* avoid path lookup for security reasons\n\n* add note that arguments are not sanitized\n\n* windows: quote args if necessary",
          "timestamp": "2025-07-30T13:30:36+02:00",
          "tree_id": "6593bb13efcc69f02e077905dd2d47340e30e417",
          "url": "https://github.com/getsentry/sentry-native/commit/93d69f4172c530d8e4ac581df2e80d97e24c7398"
        },
        "date": 1753875232397,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.728385000002163,
            "unit": "ms",
            "extra": "Min 0.703ms\nMax 0.778ms\nMean 0.734ms\nStdDev 0.029ms\nMedian 0.728ms\nCPU 0.734ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7298189999858096,
            "unit": "ms",
            "extra": "Min 0.708ms\nMax 0.736ms\nMean 0.723ms\nStdDev 0.013ms\nMedian 0.730ms\nCPU 0.723ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.1545869999831666,
            "unit": "ms",
            "extra": "Min 3.132ms\nMax 3.890ms\nMean 3.334ms\nStdDev 0.323ms\nMedian 3.155ms\nCPU 1.695ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012193000003435372,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022371999989445612,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8112850000022718,
            "unit": "ms",
            "extra": "Min 1.784ms\nMax 1.915ms\nMean 1.824ms\nStdDev 0.053ms\nMedian 1.811ms\nCPU 0.577ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "markus.hintersteiner@sentry.io",
            "name": "Markus Hintersteiner",
            "username": "markushi"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e2c2c0523ba9d14dac0c969216594c975a2a7709",
          "message": "fix(ci): Fix missing debug symbol upload for craft (#1327)\n\nThe `libsentry.so` and `libsentry-android.so` files where not uploaded to the git release sha, causing craft to be unable to find and upload these symbols to our symbol server.\n\n#skip-changelog",
          "timestamp": "2025-07-31T08:54:57+02:00",
          "tree_id": "a844f7ca37cb51840d220a0fbee724d848064acf",
          "url": "https://github.com/getsentry/sentry-native/commit/e2c2c0523ba9d14dac0c969216594c975a2a7709"
        },
        "date": 1753945036453,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7295369999980039,
            "unit": "ms",
            "extra": "Min 0.695ms\nMax 0.763ms\nMean 0.728ms\nStdDev 0.028ms\nMedian 0.730ms\nCPU 0.727ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7093589999982441,
            "unit": "ms",
            "extra": "Min 0.692ms\nMax 0.760ms\nMean 0.718ms\nStdDev 0.028ms\nMedian 0.709ms\nCPU 0.717ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.8427649999969162,
            "unit": "ms",
            "extra": "Min 2.792ms\nMax 2.950ms\nMean 2.875ms\nStdDev 0.070ms\nMedian 2.843ms\nCPU 1.518ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01201300000275296,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02187100000128339,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.022ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7563229999950636,
            "unit": "ms",
            "extra": "Min 1.734ms\nMax 1.774ms\nMean 1.753ms\nStdDev 0.016ms\nMedian 1.756ms\nCPU 0.549ms"
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
          "id": "98b38912bc3ed32e4b014cac72cd8f2aff497608",
          "message": "feat: add `(u)int64 sentry_value_t` type (#1326)\n\n* add (u)int64 sentry_value_t type\n\n* add value_to_msgpack missing switch cases\n\n* remove undefined behavior test (C99 6.3.1.4)\n\n* avoid Windows sized integer name collision\n\n* cleanup & apply code review feedback\n\n* more cleanup & remove type coercion\n\n* correctly parse (u)int64 in sentry__value_from_json\n\n* CHANGELOG.md\n\n* always send uint64 as string\n\n* simplify value_from_json integer parsing\n\n* use stdint macros instead of literals\n\n* fixed value_stringify + added unit test\n\n* Apply suggestions from code review\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>\n\n* free stringify memory + cleanup test\n\n* stringify NAN / INFINITY test with contains check\n\n* add explanation/link for writing uint64 as string\n\n* Fix macro not using parameter\n\n* update ndk gradle dependencies\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-31T12:37:10+02:00",
          "tree_id": "783406732616d145a8f2eb92a5a95fc86cfd6c7f",
          "url": "https://github.com/getsentry/sentry-native/commit/98b38912bc3ed32e4b014cac72cd8f2aff497608"
        },
        "date": 1753958423508,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7159750000198528,
            "unit": "ms",
            "extra": "Min 0.711ms\nMax 0.720ms\nMean 0.716ms\nStdDev 0.003ms\nMedian 0.716ms\nCPU 0.716ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7295700000042871,
            "unit": "ms",
            "extra": "Min 0.701ms\nMax 0.837ms\nMean 0.746ms\nStdDev 0.053ms\nMedian 0.730ms\nCPU 0.730ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9872050000108175,
            "unit": "ms",
            "extra": "Min 2.816ms\nMax 3.107ms\nMean 2.967ms\nStdDev 0.108ms\nMedian 2.987ms\nCPU 1.543ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012052999977640866,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022241000010581047,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.828384000020833,
            "unit": "ms",
            "extra": "Min 1.806ms\nMax 1.854ms\nMean 1.829ms\nStdDev 0.018ms\nMedian 1.828ms\nCPU 0.567ms"
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
          "id": "79bf2dbc59b2cf149913f819deb4777707e193cd",
          "message": "ref: use SIZE_MAX in sentry_envelope_deserialize (#1328)\n\n* ref: use SIZE_MAX in sentry_envelope_deserialize\n\n* restore payload_len < 0 check\n\n* fix length parsing vuln\n\n* Update src/sentry_envelope.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-01T13:01:26+02:00",
          "tree_id": "fa61a69295ded8986cd7509f2eed29058bb5cb08",
          "url": "https://github.com/getsentry/sentry-native/commit/79bf2dbc59b2cf149913f819deb4777707e193cd"
        },
        "date": 1754046263757,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.731187999988947,
            "unit": "ms",
            "extra": "Min 0.707ms\nMax 0.755ms\nMean 0.734ms\nStdDev 0.019ms\nMedian 0.731ms\nCPU 0.734ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7237230000214367,
            "unit": "ms",
            "extra": "Min 0.709ms\nMax 0.819ms\nMean 0.739ms\nStdDev 0.045ms\nMedian 0.724ms\nCPU 0.724ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.9921539999975266,
            "unit": "ms",
            "extra": "Min 2.928ms\nMax 3.112ms\nMean 2.997ms\nStdDev 0.071ms\nMedian 2.992ms\nCPU 1.518ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01217300001599142,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.021961000015835452,
            "unit": "ms",
            "extra": "Min 0.021ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8406269999786673,
            "unit": "ms",
            "extra": "Min 1.780ms\nMax 1.876ms\nMean 1.832ms\nStdDev 0.035ms\nMedian 1.841ms\nCPU 0.594ms"
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
          "id": "aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11",
          "message": "fix: add wildcard entries to valgrind suppression for pthread stack leak (#1330)",
          "timestamp": "2025-08-05T10:02:56+02:00",
          "tree_id": "36e09335b55bdfbcb825b1513e776d9972ae5283",
          "url": "https://github.com/getsentry/sentry-native/commit/aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11"
        },
        "date": 1754381171551,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7310449999806679,
            "unit": "ms",
            "extra": "Min 0.705ms\nMax 0.774ms\nMean 0.734ms\nStdDev 0.026ms\nMedian 0.731ms\nCPU 0.730ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7251549999978124,
            "unit": "ms",
            "extra": "Min 0.703ms\nMax 0.874ms\nMean 0.751ms\nStdDev 0.070ms\nMedian 0.725ms\nCPU 0.732ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.905801999986579,
            "unit": "ms",
            "extra": "Min 2.868ms\nMax 2.999ms\nMean 2.931ms\nStdDev 0.057ms\nMedian 2.906ms\nCPU 1.497ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012132999984260096,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022301999990759214,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.001ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8124279999938153,
            "unit": "ms",
            "extra": "Min 1.768ms\nMax 1.858ms\nMean 1.813ms\nStdDev 0.035ms\nMedian 1.812ms\nCPU 0.580ms"
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
          "id": "32e09d06b5184e562b8f2762400bf3e9e5e806f3",
          "message": "chore: Change issue template type to label (#1332)\n\n* Update issue template\n\nWe no longer use Issue Type, but Labels instead.\n\n* Update .github/ISSUE_TEMPLATE/bug_report.md\n\n---------\n\nCo-authored-by: Karl Heinz Struggl <kahest@users.noreply.github.com>",
          "timestamp": "2025-08-07T10:29:17+02:00",
          "tree_id": "ac35ea7b9bca448a239326161fcc223f8bce0c32",
          "url": "https://github.com/getsentry/sentry-native/commit/32e09d06b5184e562b8f2762400bf3e9e5e806f3"
        },
        "date": 1754555500530,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7161489999987225,
            "unit": "ms",
            "extra": "Min 0.697ms\nMax 0.901ms\nMean 0.746ms\nStdDev 0.087ms\nMedian 0.716ms\nCPU 0.726ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7163989999980913,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.735ms\nMean 0.716ms\nStdDev 0.015ms\nMedian 0.716ms\nCPU 0.715ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.007896000013943,
            "unit": "ms",
            "extra": "Min 2.864ms\nMax 3.132ms\nMean 3.001ms\nStdDev 0.123ms\nMedian 3.008ms\nCPU 1.554ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01225299999418894,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.014ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02253200000268407,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.767875000012964,
            "unit": "ms",
            "extra": "Min 1.741ms\nMax 1.810ms\nMean 1.776ms\nStdDev 0.027ms\nMedian 1.768ms\nCPU 0.548ms"
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
          "id": "305dec3449cf43975d7987510382f7149c435115",
          "message": "chore: move xbox toolchain to sentry-xbox repository (#1329)\n\n* chore: move xbox toolchain to sentry-xbox repository\n\n* update platform support docs\n\n* chore: changelog\n\n* Update CHANGELOG.md\n\n* cleanup\n\n* keep sentry.native.xbox as sdk name\n\n* restore xbox_build.md\n\n* fixup: restore sentry.h properly",
          "timestamp": "2025-08-07T12:47:11+02:00",
          "tree_id": "80b06a68781990309fa25116c73e2f3092a1aee8",
          "url": "https://github.com/getsentry/sentry-native/commit/305dec3449cf43975d7987510382f7149c435115"
        },
        "date": 1754563764883,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7082539999885284,
            "unit": "ms",
            "extra": "Min 0.699ms\nMax 0.796ms\nMean 0.727ms\nStdDev 0.040ms\nMedian 0.708ms\nCPU 0.718ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7310969999991812,
            "unit": "ms",
            "extra": "Min 0.702ms\nMax 0.862ms\nMean 0.750ms\nStdDev 0.064ms\nMedian 0.731ms\nCPU 0.733ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.030694999949901,
            "unit": "ms",
            "extra": "Min 2.903ms\nMax 3.345ms\nMean 3.060ms\nStdDev 0.168ms\nMedian 3.031ms\nCPU 1.528ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012272999981632893,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.02277300001196636,
            "unit": "ms",
            "extra": "Min 0.023ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.000ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.8080829999576054,
            "unit": "ms",
            "extra": "Min 1.785ms\nMax 1.853ms\nMean 1.817ms\nStdDev 0.028ms\nMedian 1.808ms\nCPU 0.565ms"
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
          "id": "6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee",
          "message": "feat: manage trace life-cycle (#1270)\n\n* feat: manage trace duration\n\n* format after web-editor merge\n\n* clean up regenerate_trace() test after merge\n\n* prevent warning-as-error\n\n* add changelog\n\n* ensure trace_managed is turned off when regenerating a trace\n\n* update breaking changes\n\n* test that transaction no longer act as trace boundaries after a user regenerates a trace\n\n* highlight the topic of automatic trace boundaries in the docs for `sentry_set_trace()` and `sentry_regenerate_trace()`\n\n* Update tests/unit/test_tracing.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* remove duplicate includes in sentry_core.c\n\n* tighten regenerate part of the test against the trace generated during the previous transaction\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-08T12:24:46+02:00",
          "tree_id": "13cad9add2e8e83d579991f58deca96160144236",
          "url": "https://github.com/getsentry/sentry-native/commit/6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee"
        },
        "date": 1754648873748,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7163860000218847,
            "unit": "ms",
            "extra": "Min 0.702ms\nMax 0.738ms\nMean 0.717ms\nStdDev 0.014ms\nMedian 0.716ms\nCPU 0.716ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7552170000053593,
            "unit": "ms",
            "extra": "Min 0.722ms\nMax 0.784ms\nMean 0.752ms\nStdDev 0.023ms\nMedian 0.755ms\nCPU 0.752ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.881699999761622,
            "unit": "ms",
            "extra": "Min 2.842ms\nMax 3.151ms\nMean 2.934ms\nStdDev 0.125ms\nMedian 2.882ms\nCPU 1.553ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012302999948587967,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022822999653726583,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.024ms\nMean 0.023ms\nStdDev 0.001ms\nMedian 0.023ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.7872959997475846,
            "unit": "ms",
            "extra": "Min 1.764ms\nMax 1.879ms\nMean 1.811ms\nStdDev 0.048ms\nMedian 1.787ms\nCPU 0.543ms"
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
          "id": "d5c34db05c6d8cca302c2dface5063b0488df5be",
          "message": "craft: only include zip artifacts for github target (#1333)\n\n#skip-changelog",
          "timestamp": "2025-08-08T13:53:16+02:00",
          "tree_id": "bbe859661565e9a10a808263d75f3a5c6cf8aecf",
          "url": "https://github.com/getsentry/sentry-native/commit/d5c34db05c6d8cca302c2dface5063b0488df5be"
        },
        "date": 1754654182679,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7228540000028261,
            "unit": "ms",
            "extra": "Min 0.709ms\nMax 0.737ms\nMean 0.722ms\nStdDev 0.010ms\nMedian 0.723ms\nCPU 0.721ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.714648000013085,
            "unit": "ms",
            "extra": "Min 0.696ms\nMax 0.721ms\nMean 0.710ms\nStdDev 0.011ms\nMedian 0.715ms\nCPU 0.709ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 2.876587000002928,
            "unit": "ms",
            "extra": "Min 2.830ms\nMax 3.616ms\nMean 3.042ms\nStdDev 0.330ms\nMedian 2.877ms\nCPU 1.521ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012101999999458712,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.022242000000005646,
            "unit": "ms",
            "extra": "Min 0.022ms\nMax 0.023ms\nMean 0.022ms\nStdDev 0.000ms\nMedian 0.022ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 1.766112000012754,
            "unit": "ms",
            "extra": "Min 1.750ms\nMax 1.845ms\nMean 1.785ms\nStdDev 0.038ms\nMedian 1.766ms\nCPU 0.557ms"
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
          "id": "18d8e4b4fc3ddbd449499a5da7fd4ee84e65b6de",
          "message": "Merge branch 'release/0.10.0'",
          "timestamp": "2025-08-08T12:38:45Z",
          "tree_id": "533a84c532977d2f1d9b3ef9c6a483a7e8a64da0",
          "url": "https://github.com/getsentry/sentry-native/commit/18d8e4b4fc3ddbd449499a5da7fd4ee84e65b6de"
        },
        "date": 1754656923519,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 0.7236960000227555,
            "unit": "ms",
            "extra": "Min 0.714ms\nMax 0.763ms\nMean 0.733ms\nStdDev 0.020ms\nMedian 0.724ms\nCPU 0.733ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 0.7262510000032307,
            "unit": "ms",
            "extra": "Min 0.706ms\nMax 0.739ms\nMean 0.724ms\nStdDev 0.014ms\nMedian 0.726ms\nCPU 0.724ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 3.4467549999988023,
            "unit": "ms",
            "extra": "Min 3.245ms\nMax 3.637ms\nMean 3.462ms\nStdDev 0.147ms\nMedian 3.447ms\nCPU 1.809ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012432999994871352,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.024125999999569103,
            "unit": "ms",
            "extra": "Min 0.023ms\nMax 0.026ms\nMean 0.024ms\nStdDev 0.001ms\nMedian 0.024ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 2.081376000006685,
            "unit": "ms",
            "extra": "Min 2.005ms\nMax 2.307ms\nMean 2.136ms\nStdDev 0.121ms\nMedian 2.081ms\nCPU 0.645ms"
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
        "date": 1747312258003,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.947916999971767,
            "unit": "ms",
            "extra": "Min 2.683ms\nMax 4.373ms\nMean 3.168ms\nStdDev 0.682ms\nMedian 2.948ms\nCPU 1.622ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.139959000009185,
            "unit": "ms",
            "extra": "Min 2.994ms\nMax 3.500ms\nMean 3.202ms\nStdDev 0.227ms\nMedian 3.140ms\nCPU 1.785ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.566999999980453,
            "unit": "ms",
            "extra": "Min 10.010ms\nMax 12.377ms\nMean 10.799ms\nStdDev 0.945ms\nMedian 10.567ms\nCPU 3.868ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02891700000873243,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.042ms\nMean 0.026ms\nStdDev 0.016ms\nMedian 0.029ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.21441700005198072,
            "unit": "ms",
            "extra": "Min 0.202ms\nMax 0.265ms\nMean 0.224ms\nStdDev 0.026ms\nMedian 0.214ms\nCPU 0.223ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.4449169999770675,
            "unit": "ms",
            "extra": "Min 5.165ms\nMax 5.992ms\nMean 5.578ms\nStdDev 0.376ms\nMedian 5.445ms\nCPU 0.776ms"
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
          "id": "426c60558d68ac13204d9c9bfb64f5284cb36d7e",
          "message": "docs: windows test and formatter runners (#1247)",
          "timestamp": "2025-05-15T17:27:13+02:00",
          "tree_id": "7a79c6dc37e714afa484243ab15e43e50d3a99fb",
          "url": "https://github.com/getsentry/sentry-native/commit/426c60558d68ac13204d9c9bfb64f5284cb36d7e"
        },
        "date": 1747323007060,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.823833000036302,
            "unit": "ms",
            "extra": "Min 2.673ms\nMax 4.335ms\nMean 3.123ms\nStdDev 0.693ms\nMedian 2.824ms\nCPU 1.691ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.9852079999841408,
            "unit": "ms",
            "extra": "Min 2.887ms\nMax 4.272ms\nMean 3.346ms\nStdDev 0.596ms\nMedian 2.985ms\nCPU 1.884ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.044249999997646,
            "unit": "ms",
            "extra": "Min 10.642ms\nMax 15.248ms\nMean 12.198ms\nStdDev 2.022ms\nMedian 11.044ms\nCPU 4.344ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.029374999996889528,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.033ms\nMean 0.022ms\nStdDev 0.012ms\nMedian 0.029ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2806669999699807,
            "unit": "ms",
            "extra": "Min 0.219ms\nMax 0.968ms\nMean 0.423ms\nStdDev 0.311ms\nMedian 0.281ms\nCPU 0.284ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.725334000009298,
            "unit": "ms",
            "extra": "Min 5.781ms\nMax 9.511ms\nMean 7.115ms\nStdDev 1.552ms\nMedian 6.725ms\nCPU 0.892ms"
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
          "id": "c653de396546b870f563f320f15362282a2432b5",
          "message": "chore: update gradle scripts to current AGP/SDK usage (#1256)",
          "timestamp": "2025-05-27T15:08:13+02:00",
          "tree_id": "f3df760bb7c0e2f55580d7011626ef29a7b77cff",
          "url": "https://github.com/getsentry/sentry-native/commit/c653de396546b870f563f320f15362282a2432b5"
        },
        "date": 1748351474224,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.611833000036313,
            "unit": "ms",
            "extra": "Min 3.147ms\nMax 7.386ms\nMean 4.228ms\nStdDev 1.786ms\nMedian 3.612ms\nCPU 2.264ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.199040999992576,
            "unit": "ms",
            "extra": "Min 3.397ms\nMax 9.762ms\nMean 5.150ms\nStdDev 2.610ms\nMedian 4.199ms\nCPU 2.896ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.679499999956988,
            "unit": "ms",
            "extra": "Min 9.668ms\nMax 19.168ms\nMean 12.565ms\nStdDev 3.800ms\nMedian 11.679ms\nCPU 4.809ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.033333000033053395,
            "unit": "ms",
            "extra": "Min 0.026ms\nMax 0.039ms\nMean 0.033ms\nStdDev 0.005ms\nMedian 0.033ms\nCPU 0.033ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2098749999959182,
            "unit": "ms",
            "extra": "Min 0.186ms\nMax 0.233ms\nMean 0.209ms\nStdDev 0.017ms\nMedian 0.210ms\nCPU 0.208ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.05941700001722,
            "unit": "ms",
            "extra": "Min 5.401ms\nMax 9.772ms\nMean 6.706ms\nStdDev 1.783ms\nMedian 6.059ms\nCPU 1.079ms"
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
          "id": "aa1f1b79dbf31d9576dd9573e8c863dd685ea13b",
          "message": "fix(ndk): correct interpolation of new `project.layout.buildDirectory` property (#1258)",
          "timestamp": "2025-05-28T10:09:46+02:00",
          "tree_id": "4245ae38cbb06385b4a7e795849313dfe997dac8",
          "url": "https://github.com/getsentry/sentry-native/commit/aa1f1b79dbf31d9576dd9573e8c863dd685ea13b"
        },
        "date": 1748420121638,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.248458000030041,
            "unit": "ms",
            "extra": "Min 3.143ms\nMax 5.220ms\nMean 3.811ms\nStdDev 0.924ms\nMedian 3.248ms\nCPU 1.957ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1237079999755224,
            "unit": "ms",
            "extra": "Min 2.947ms\nMax 3.603ms\nMean 3.186ms\nStdDev 0.247ms\nMedian 3.124ms\nCPU 1.831ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.36166700001695,
            "unit": "ms",
            "extra": "Min 9.724ms\nMax 12.332ms\nMean 11.117ms\nStdDev 1.029ms\nMedian 11.362ms\nCPU 3.904ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03216699997210526,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.040ms\nMean 0.025ms\nStdDev 0.014ms\nMedian 0.032ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24091699998507465,
            "unit": "ms",
            "extra": "Min 0.216ms\nMax 0.286ms\nMean 0.245ms\nStdDev 0.025ms\nMedian 0.241ms\nCPU 0.244ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.655291999971723,
            "unit": "ms",
            "extra": "Min 5.411ms\nMax 6.430ms\nMean 5.751ms\nStdDev 0.396ms\nMedian 5.655ms\nCPU 0.790ms"
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
          "id": "6ca83891fdea3f141df66eb6a2f148ef465463c2",
          "message": "fix: use custom page allocator on ps5 (#1257)",
          "timestamp": "2025-05-28T10:16:54+02:00",
          "tree_id": "839d7d4472a60b05ccb61e0105979c5205dd1bf1",
          "url": "https://github.com/getsentry/sentry-native/commit/6ca83891fdea3f141df66eb6a2f148ef465463c2"
        },
        "date": 1748420445007,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.0365420000180166,
            "unit": "ms",
            "extra": "Min 2.841ms\nMax 3.436ms\nMean 3.067ms\nStdDev 0.234ms\nMedian 3.037ms\nCPU 1.698ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.366792000008445,
            "unit": "ms",
            "extra": "Min 3.326ms\nMax 5.293ms\nMean 3.766ms\nStdDev 0.856ms\nMedian 3.367ms\nCPU 2.050ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.694666999995661,
            "unit": "ms",
            "extra": "Min 9.894ms\nMax 16.732ms\nMean 12.043ms\nStdDev 2.775ms\nMedian 10.695ms\nCPU 4.553ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.007791000030010764,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.065ms\nMean 0.027ms\nStdDev 0.027ms\nMedian 0.008ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.20962499996812767,
            "unit": "ms",
            "extra": "Min 0.186ms\nMax 0.432ms\nMean 0.248ms\nStdDev 0.103ms\nMedian 0.210ms\nCPU 0.248ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.7798749999733445,
            "unit": "ms",
            "extra": "Min 5.509ms\nMax 6.843ms\nMean 5.925ms\nStdDev 0.528ms\nMedian 5.780ms\nCPU 0.751ms"
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
          "id": "a1b3947584935eea5680ba54e8914ec913509566",
          "message": "fix: limit proguard rules in the NDK package to local namespaces (#1250)\n\n* fix: limit proguard rules in the NDK package to local namespaces\n\n* rewörding",
          "timestamp": "2025-06-02T08:08:51+02:00",
          "tree_id": "ad71de44e8f71235e89c6e97cd355467f52ddf91",
          "url": "https://github.com/getsentry/sentry-native/commit/a1b3947584935eea5680ba54e8914ec913509566"
        },
        "date": 1748844702785,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.5021250000681903,
            "unit": "ms",
            "extra": "Min 3.046ms\nMax 4.959ms\nMean 3.736ms\nStdDev 0.728ms\nMedian 3.502ms\nCPU 2.190ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.121582999938255,
            "unit": "ms",
            "extra": "Min 2.999ms\nMax 3.455ms\nMean 3.158ms\nStdDev 0.187ms\nMedian 3.122ms\nCPU 1.751ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 12.38633299999492,
            "unit": "ms",
            "extra": "Min 10.676ms\nMax 29.900ms\nMean 16.021ms\nStdDev 7.934ms\nMedian 12.386ms\nCPU 4.969ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03170900004079158,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.045ms\nMean 0.029ms\nStdDev 0.014ms\nMedian 0.032ms\nCPU 0.029ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.18375000013293175,
            "unit": "ms",
            "extra": "Min 0.151ms\nMax 0.227ms\nMean 0.186ms\nStdDev 0.027ms\nMedian 0.184ms\nCPU 0.185ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.877457999986291,
            "unit": "ms",
            "extra": "Min 5.599ms\nMax 9.767ms\nMean 6.685ms\nStdDev 1.743ms\nMedian 5.877ms\nCPU 1.192ms"
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
          "id": "9c0950343b3531c797b4c1c87ae6f5b5d1281ed3",
          "message": "fix: close file and return 0 on success when writing raw envelopes (#1260)",
          "timestamp": "2025-06-02T10:54:33+02:00",
          "tree_id": "2d6515f02317a16a2f5e417fe7850a1042cefa60",
          "url": "https://github.com/getsentry/sentry-native/commit/9c0950343b3531c797b4c1c87ae6f5b5d1281ed3"
        },
        "date": 1748854653852,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.7642080000068745,
            "unit": "ms",
            "extra": "Min 3.093ms\nMax 4.235ms\nMean 3.677ms\nStdDev 0.439ms\nMedian 3.764ms\nCPU 2.109ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.584708000005321,
            "unit": "ms",
            "extra": "Min 3.258ms\nMax 7.784ms\nMean 4.397ms\nStdDev 1.911ms\nMedian 3.585ms\nCPU 2.452ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 14.242416000001867,
            "unit": "ms",
            "extra": "Min 12.698ms\nMax 16.782ms\nMean 14.493ms\nStdDev 1.480ms\nMedian 14.242ms\nCPU 4.801ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03325000000131695,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.077ms\nMean 0.038ms\nStdDev 0.024ms\nMedian 0.033ms\nCPU 0.037ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2642910000076881,
            "unit": "ms",
            "extra": "Min 0.231ms\nMax 1.168ms\nMean 0.434ms\nStdDev 0.411ms\nMedian 0.264ms\nCPU 0.295ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.842874999970718,
            "unit": "ms",
            "extra": "Min 7.070ms\nMax 18.683ms\nMean 11.693ms\nStdDev 5.024ms\nMedian 8.843ms\nCPU 1.427ms"
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
          "id": "0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a",
          "message": "ci: pin kcov to v43 (#1265)",
          "timestamp": "2025-06-03T10:19:39+02:00",
          "tree_id": "71da9cd13983c4e189589ad750e36b9fe96bccbc",
          "url": "https://github.com/getsentry/sentry-native/commit/0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a"
        },
        "date": 1748939081392,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.8989170000149898,
            "unit": "ms",
            "extra": "Min 2.731ms\nMax 8.640ms\nMean 4.583ms\nStdDev 2.321ms\nMedian 3.899ms\nCPU 2.597ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.3768330000043534,
            "unit": "ms",
            "extra": "Min 2.960ms\nMax 5.092ms\nMean 3.781ms\nStdDev 0.841ms\nMedian 3.377ms\nCPU 2.158ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.841207999978906,
            "unit": "ms",
            "extra": "Min 11.412ms\nMax 96.951ms\nMean 34.184ms\nStdDev 35.741ms\nMedian 20.841ms\nCPU 4.138ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02179199998408876,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.039ms\nMean 0.022ms\nStdDev 0.012ms\nMedian 0.022ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.20183300000553572,
            "unit": "ms",
            "extra": "Min 0.191ms\nMax 0.274ms\nMean 0.214ms\nStdDev 0.034ms\nMedian 0.202ms\nCPU 0.213ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.291250000037053,
            "unit": "ms",
            "extra": "Min 6.343ms\nMax 10.060ms\nMean 7.622ms\nStdDev 1.526ms\nMedian 7.291ms\nCPU 0.817ms"
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
          "id": "1fe91217f1ce839f46ef398bdf1f3bf58d99a0da",
          "message": "ci: use Alpine Linux Docker image (#1261)",
          "timestamp": "2025-06-03T11:10:02+02:00",
          "tree_id": "2a2a6c8a0fff494a2d8eb478d6f7350cc26c2125",
          "url": "https://github.com/getsentry/sentry-native/commit/1fe91217f1ce839f46ef398bdf1f3bf58d99a0da"
        },
        "date": 1748941973502,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.4927920000029644,
            "unit": "ms",
            "extra": "Min 3.066ms\nMax 8.684ms\nMean 4.447ms\nStdDev 2.376ms\nMedian 3.493ms\nCPU 2.372ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.3508330000131537,
            "unit": "ms",
            "extra": "Min 2.995ms\nMax 4.150ms\nMean 3.443ms\nStdDev 0.426ms\nMedian 3.351ms\nCPU 1.964ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.44716700003346,
            "unit": "ms",
            "extra": "Min 14.279ms\nMax 33.044ms\nMean 20.792ms\nStdDev 7.591ms\nMedian 18.447ms\nCPU 6.769ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.036125000008269126,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.041ms\nMean 0.032ms\nStdDev 0.013ms\nMedian 0.036ms\nCPU 0.031ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3902079999988928,
            "unit": "ms",
            "extra": "Min 0.218ms\nMax 0.454ms\nMean 0.368ms\nStdDev 0.090ms\nMedian 0.390ms\nCPU 0.368ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.23579200000313,
            "unit": "ms",
            "extra": "Min 7.384ms\nMax 18.386ms\nMean 11.713ms\nStdDev 4.771ms\nMedian 10.236ms\nCPU 1.085ms"
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
          "id": "fc6a314033b1df0ddb9a9ecb51b3268deb7572cb",
          "message": "fix: respect event data when applying/merging scope data (#1253)",
          "timestamp": "2025-06-04T12:32:03+02:00",
          "tree_id": "04848bc8968f055481f86c213cbd3a2f3e6dafa1",
          "url": "https://github.com/getsentry/sentry-native/commit/fc6a314033b1df0ddb9a9ecb51b3268deb7572cb"
        },
        "date": 1749033296674,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.818332999993345,
            "unit": "ms",
            "extra": "Min 2.659ms\nMax 4.376ms\nMean 3.144ms\nStdDev 0.706ms\nMedian 2.818ms\nCPU 1.773ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.9880840000089393,
            "unit": "ms",
            "extra": "Min 2.684ms\nMax 3.752ms\nMean 3.108ms\nStdDev 0.436ms\nMedian 2.988ms\nCPU 1.870ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 13.355165999996643,
            "unit": "ms",
            "extra": "Min 11.130ms\nMax 22.045ms\nMean 14.782ms\nStdDev 4.475ms\nMedian 13.355ms\nCPU 5.225ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.032959000009213923,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.041ms\nMean 0.027ms\nStdDev 0.014ms\nMedian 0.033ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2259160000335214,
            "unit": "ms",
            "extra": "Min 0.186ms\nMax 0.281ms\nMean 0.232ms\nStdDev 0.036ms\nMedian 0.226ms\nCPU 0.232ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.536749999976109,
            "unit": "ms",
            "extra": "Min 5.030ms\nMax 15.518ms\nMean 7.799ms\nStdDev 4.448ms\nMedian 5.537ms\nCPU 0.823ms"
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
          "id": "31a8ee35ab3a77fcd201535ea814a3611ad34102",
          "message": "feat: add PS SDK transport support (#1262)\n\n* feat: allow downstream SDKs to implement custom transport\n\n* add \"pshttp\" support to the list of supported ones",
          "timestamp": "2025-06-04T14:10:02+02:00",
          "tree_id": "397dcdefbc9b7cc6aafa6f596959832e93704850",
          "url": "https://github.com/getsentry/sentry-native/commit/31a8ee35ab3a77fcd201535ea814a3611ad34102"
        },
        "date": 1749039198277,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.588291000004574,
            "unit": "ms",
            "extra": "Min 4.176ms\nMax 4.892ms\nMean 4.558ms\nStdDev 0.262ms\nMedian 4.588ms\nCPU 2.462ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.936999999927139,
            "unit": "ms",
            "extra": "Min 4.255ms\nMax 5.257ms\nMean 4.880ms\nStdDev 0.396ms\nMedian 4.937ms\nCPU 2.925ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 13.18025000000489,
            "unit": "ms",
            "extra": "Min 11.998ms\nMax 36.407ms\nMean 20.014ms\nStdDev 10.984ms\nMedian 13.180ms\nCPU 7.832ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03333299991936656,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.039ms\nMean 0.026ms\nStdDev 0.016ms\nMedian 0.033ms\nCPU 0.026ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3034579999621201,
            "unit": "ms",
            "extra": "Min 0.259ms\nMax 0.338ms\nMean 0.293ms\nStdDev 0.034ms\nMedian 0.303ms\nCPU 0.293ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.204999999999018,
            "unit": "ms",
            "extra": "Min 6.894ms\nMax 7.360ms\nMean 7.164ms\nStdDev 0.172ms\nMedian 7.205ms\nCPU 1.137ms"
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
          "id": "54cede931f9a76e291be9083d56702c7d20150cd",
          "message": "feat: Add support for capturing events with local scopes (#1248)\n\n* wip: local scopes\n\n* merge breadcrumbs\n\n* add sentry_scope_set_trace\n\n* add sentry_scope_set_fingerprints()\n\n* check fingerprints value type\n\n* document sentry_scope_set_fingerprints() expected type\n\n* Revert sentry_scope_set_trace/transaction\n\n> Transactions/spans do not make sense in this setup since they aren't\n> cloned and cannot be retrieved to create children.\n\n* sentry_malloc -> SENTRY_MAKE\n\n* fix comparing null timestamps when merging breadcrumbs\n\n* take ownership\n\n* update example\n\n* partial revert of unit test changes in a48fea\n\ndon't assume any specific order for breadcrumbs with missing breadcrumbs\n\n* warn once if any breadcrumbs were missing timestamps\n\n* error handling for sentry_value_append()",
          "timestamp": "2025-06-04T16:29:38+02:00",
          "tree_id": "2c4e06d37f7f702c92c3effcb198b108f06a6b70",
          "url": "https://github.com/getsentry/sentry-native/commit/54cede931f9a76e291be9083d56702c7d20150cd"
        },
        "date": 1749047518389,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.7737500000171167,
            "unit": "ms",
            "extra": "Min 2.608ms\nMax 4.411ms\nMean 3.048ms\nStdDev 0.767ms\nMedian 2.774ms\nCPU 1.544ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.6900839999939308,
            "unit": "ms",
            "extra": "Min 2.593ms\nMax 2.775ms\nMean 2.688ms\nStdDev 0.082ms\nMedian 2.690ms\nCPU 1.568ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 8.153292000031342,
            "unit": "ms",
            "extra": "Min 7.939ms\nMax 8.606ms\nMean 8.240ms\nStdDev 0.278ms\nMedian 8.153ms\nCPU 2.976ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01404200003207734,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.017ms\nMean 0.013ms\nStdDev 0.004ms\nMedian 0.014ms\nCPU 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.16270899999426547,
            "unit": "ms",
            "extra": "Min 0.151ms\nMax 0.202ms\nMean 0.172ms\nStdDev 0.020ms\nMedian 0.163ms\nCPU 0.172ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 4.506124999977601,
            "unit": "ms",
            "extra": "Min 4.379ms\nMax 7.525ms\nMean 5.134ms\nStdDev 1.347ms\nMedian 4.506ms\nCPU 0.619ms"
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
          "id": "c56ebdea24486771e28853b78afcd4f6d8ab4833",
          "message": "feat: `crashpad_wait_for_upload` Windows support (#1255)",
          "timestamp": "2025-06-05T18:46:32+02:00",
          "tree_id": "2b213da0b6f44cb2588ac5cd94f4cf3006cbccf2",
          "url": "https://github.com/getsentry/sentry-native/commit/c56ebdea24486771e28853b78afcd4f6d8ab4833"
        },
        "date": 1749142180262,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.203916999983903,
            "unit": "ms",
            "extra": "Min 3.205ms\nMax 6.183ms\nMean 4.415ms\nStdDev 1.087ms\nMedian 4.204ms\nCPU 2.396ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.406958000008899,
            "unit": "ms",
            "extra": "Min 4.138ms\nMax 6.572ms\nMean 5.364ms\nStdDev 0.899ms\nMedian 5.407ms\nCPU 2.981ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 15.560708000009527,
            "unit": "ms",
            "extra": "Min 10.722ms\nMax 17.057ms\nMean 14.075ms\nStdDev 2.980ms\nMedian 15.561ms\nCPU 5.012ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.0327089999814234,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.035ms\nMean 0.025ms\nStdDev 0.013ms\nMedian 0.033ms\nCPU 0.024ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.26099999996631595,
            "unit": "ms",
            "extra": "Min 0.221ms\nMax 0.617ms\nMean 0.343ms\nStdDev 0.165ms\nMedian 0.261ms\nCPU 0.343ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.126833000038914,
            "unit": "ms",
            "extra": "Min 8.703ms\nMax 14.792ms\nMean 10.804ms\nStdDev 2.682ms\nMedian 9.127ms\nCPU 1.386ms"
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
          "id": "9b29c7996e06c200c23c199d95844be8366b16cc",
          "message": "Merge branch 'release/0.9.0'",
          "timestamp": "2025-06-05T17:31:05Z",
          "tree_id": "522307788bf54fcc5ce2e6fa66c7c423134a840c",
          "url": "https://github.com/getsentry/sentry-native/commit/9b29c7996e06c200c23c199d95844be8366b16cc"
        },
        "date": 1749144874645,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 5.594958000017414,
            "unit": "ms",
            "extra": "Min 4.040ms\nMax 9.860ms\nMean 6.068ms\nStdDev 2.406ms\nMedian 5.595ms\nCPU 3.572ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.239875000000893,
            "unit": "ms",
            "extra": "Min 4.419ms\nMax 7.397ms\nMean 5.742ms\nStdDev 1.294ms\nMedian 5.240ms\nCPU 3.313ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.804250000021284,
            "unit": "ms",
            "extra": "Min 13.067ms\nMax 23.212ms\nMean 18.176ms\nStdDev 4.651ms\nMedian 19.804ms\nCPU 6.393ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.039125000000694854,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.169ms\nMean 0.061ms\nStdDev 0.062ms\nMedian 0.039ms\nCPU 0.060ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.28687500002888555,
            "unit": "ms",
            "extra": "Min 0.268ms\nMax 0.317ms\nMean 0.293ms\nStdDev 0.019ms\nMedian 0.287ms\nCPU 0.292ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.30687500001659,
            "unit": "ms",
            "extra": "Min 7.779ms\nMax 30.992ms\nMean 13.873ms\nStdDev 9.913ms\nMedian 8.307ms\nCPU 1.142ms"
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
          "id": "16fa6892134f162ddbfaaa94b230ca5cd8564e0a",
          "message": "fix: introduce malloc/MAKE rv checks if missing (#1234)\n\n+ ensure that none of the test runs into a segfault by asserting on malloc return paths that propagate",
          "timestamp": "2025-06-12T13:29:34+02:00",
          "tree_id": "f8c1c235ae6bbe2d43730436229d5ee7e5ffc7e2",
          "url": "https://github.com/getsentry/sentry-native/commit/16fa6892134f162ddbfaaa94b230ca5cd8564e0a"
        },
        "date": 1749727971483,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.219165999979623,
            "unit": "ms",
            "extra": "Min 3.087ms\nMax 4.996ms\nMean 3.644ms\nStdDev 0.792ms\nMedian 3.219ms\nCPU 2.151ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.754833999958464,
            "unit": "ms",
            "extra": "Min 3.733ms\nMax 7.690ms\nMean 5.951ms\nStdDev 1.510ms\nMedian 5.755ms\nCPU 3.622ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.791958000008435,
            "unit": "ms",
            "extra": "Min 16.743ms\nMax 20.023ms\nMean 18.632ms\nStdDev 1.258ms\nMedian 18.792ms\nCPU 6.216ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.062124999999468855,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.163ms\nMean 0.066ms\nStdDev 0.063ms\nMedian 0.062ms\nCPU 0.065ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24074999998902058,
            "unit": "ms",
            "extra": "Min 0.211ms\nMax 0.567ms\nMean 0.313ms\nStdDev 0.147ms\nMedian 0.241ms\nCPU 0.313ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.79858400004241,
            "unit": "ms",
            "extra": "Min 9.541ms\nMax 26.964ms\nMean 14.451ms\nStdDev 7.257ms\nMedian 10.799ms\nCPU 1.400ms"
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
          "id": "1bf8db1646a8e6d3b497f3d173368192b75e05ba",
          "message": "ci: drop windows-2019 runner images (#1274)",
          "timestamp": "2025-06-13T11:51:12+02:00",
          "tree_id": "732b83a0139c34a3b7fdfcb4247c8600cd7022b6",
          "url": "https://github.com/getsentry/sentry-native/commit/1bf8db1646a8e6d3b497f3d173368192b75e05ba"
        },
        "date": 1749808452075,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.2723750000513974,
            "unit": "ms",
            "extra": "Min 2.704ms\nMax 4.173ms\nMean 3.410ms\nStdDev 0.645ms\nMedian 3.272ms\nCPU 1.929ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.5186659999908443,
            "unit": "ms",
            "extra": "Min 3.036ms\nMax 4.255ms\nMean 3.533ms\nStdDev 0.482ms\nMedian 3.519ms\nCPU 2.129ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.86683399998401,
            "unit": "ms",
            "extra": "Min 13.015ms\nMax 22.553ms\nMean 17.114ms\nStdDev 3.687ms\nMedian 16.867ms\nCPU 5.229ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.04012499994132668,
            "unit": "ms",
            "extra": "Min 0.031ms\nMax 0.045ms\nMean 0.038ms\nStdDev 0.007ms\nMedian 0.040ms\nCPU 0.037ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.19466599997031153,
            "unit": "ms",
            "extra": "Min 0.148ms\nMax 0.363ms\nMean 0.226ms\nStdDev 0.082ms\nMedian 0.195ms\nCPU 0.225ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.96445800001311,
            "unit": "ms",
            "extra": "Min 7.783ms\nMax 14.141ms\nMean 10.431ms\nStdDev 2.304ms\nMedian 9.964ms\nCPU 1.504ms"
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
          "id": "7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8",
          "message": "feat: Support modifying attachments after init (continued) (#1266)\n\n* feat: Support modifying attachments after init\n\nMoves the attachments to the scope, and adds `sentry_add_attachment` and\n`sentry_remove_attachment` and wstr variants that modify this attachment\nlist after calling init. Attachments are identified by their path.\n\n* feat: pass added and removed attachments to the backend\n\n* add `_n`\n\n* scope api\n\n* merge & apply attachments\n\n* update note on attachments\n\n* integration tests\n\n* Update README.md\n\n* Update CHANGELOG.md\n\n* Apply suggestions from code review\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* remove ticks\n\n* Apply more suggestions from code review\n\n* De-duplicate envelope attachment code\n\n- remove sentry__apply_attachments_to_envelope\n- add sentry__envelope_add_attachments\n- reuse sentry__envelope_add_attachment\n\n* sentry_add_attachment -> sentry_add_attachment_path\n\n* Update CHANGELOG.md\n\n* fixup: missed rename\n\n* fixup: another missed rename\n\n* remove_attachmentw() without _path\n\n* revise sentry_attach_file & removal\n\n* fix windows\n\n* Update CHANGELOG.md\n\n* clean up\n\n* fix attachments_add_remove on windows\n\n* Update CHANGELOG.md & NOTE on attachments\n\n* Update external/crashpad\n\n---------\n\nCo-authored-by: Arpad Borsos <arpad.borsos@googlemail.com>\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-06-13T14:02:33+02:00",
          "tree_id": "cb94ac9185e31f5e47c7ebc651235c14006bf020",
          "url": "https://github.com/getsentry/sentry-native/commit/7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8"
        },
        "date": 1749816343878,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.520042000024205,
            "unit": "ms",
            "extra": "Min 3.546ms\nMax 7.042ms\nMean 4.827ms\nStdDev 1.329ms\nMedian 4.520ms\nCPU 2.710ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.087958000013714,
            "unit": "ms",
            "extra": "Min 3.600ms\nMax 5.648ms\nMean 4.520ms\nStdDev 1.002ms\nMedian 4.088ms\nCPU 2.438ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.04466699996965,
            "unit": "ms",
            "extra": "Min 12.985ms\nMax 22.993ms\nMean 18.365ms\nStdDev 3.710ms\nMedian 18.045ms\nCPU 5.600ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03741699998727199,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.079ms\nMean 0.041ms\nStdDev 0.025ms\nMedian 0.037ms\nCPU 0.041ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.32454099999768005,
            "unit": "ms",
            "extra": "Min 0.239ms\nMax 0.465ms\nMean 0.325ms\nStdDev 0.089ms\nMedian 0.325ms\nCPU 0.325ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.775292000005265,
            "unit": "ms",
            "extra": "Min 7.041ms\nMax 17.472ms\nMean 11.238ms\nStdDev 3.874ms\nMedian 10.775ms\nCPU 1.199ms"
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
          "id": "d823acbc116d19393c3552a123ab7e483c383b03",
          "message": "docs: sync return values of AIX dladdr with implementation (#1273)",
          "timestamp": "2025-06-13T15:22:47+02:00",
          "tree_id": "abde47443e4fe5f6a7a7f6e22cff13dfa090ffb9",
          "url": "https://github.com/getsentry/sentry-native/commit/d823acbc116d19393c3552a123ab7e483c383b03"
        },
        "date": 1749821173340,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.715333999991799,
            "unit": "ms",
            "extra": "Min 3.082ms\nMax 5.362ms\nMean 3.865ms\nStdDev 0.927ms\nMedian 3.715ms\nCPU 2.266ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 6.930332999957045,
            "unit": "ms",
            "extra": "Min 3.517ms\nMax 8.713ms\nMean 6.873ms\nStdDev 2.080ms\nMedian 6.930ms\nCPU 3.763ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.621041000026707,
            "unit": "ms",
            "extra": "Min 14.652ms\nMax 41.977ms\nMean 24.447ms\nStdDev 10.895ms\nMedian 20.621ms\nCPU 7.723ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.040000000012696546,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.048ms\nMean 0.035ms\nStdDev 0.014ms\nMedian 0.040ms\nCPU 0.034ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.297375000059219,
            "unit": "ms",
            "extra": "Min 0.255ms\nMax 0.421ms\nMean 0.332ms\nStdDev 0.071ms\nMedian 0.297ms\nCPU 0.331ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 12.207125000031738,
            "unit": "ms",
            "extra": "Min 9.458ms\nMax 15.656ms\nMean 12.272ms\nStdDev 2.482ms\nMedian 12.207ms\nCPU 1.390ms"
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
          "id": "931c468cb6dcb8856c60cd113ef044d9881ef62e",
          "message": "test: define individual unit tests for CTEST (#1244)\n\n* test: define individual unit tests for CTEST\n\n* chore: add SENTRY_CTEST_INDIVIDUAL option\n\n* rename the aggregate test target to \"unit-tests\"",
          "timestamp": "2025-06-16T12:19:00+02:00",
          "tree_id": "a5600d12fbaa848014d06846b9b5d5ea7bf1a63a",
          "url": "https://github.com/getsentry/sentry-native/commit/931c468cb6dcb8856c60cd113ef044d9881ef62e"
        },
        "date": 1750069355646,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.9974589999806085,
            "unit": "ms",
            "extra": "Min 4.131ms\nMax 19.975ms\nMean 8.037ms\nStdDev 6.745ms\nMedian 4.997ms\nCPU 4.751ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.8789580000061505,
            "unit": "ms",
            "extra": "Min 3.729ms\nMax 5.059ms\nMean 4.514ms\nStdDev 0.663ms\nMedian 4.879ms\nCPU 2.738ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.415666000019428,
            "unit": "ms",
            "extra": "Min 14.117ms\nMax 34.322ms\nMean 21.834ms\nStdDev 7.555ms\nMedian 20.416ms\nCPU 6.630ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.04262500010554504,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.048ms\nMean 0.032ms\nStdDev 0.018ms\nMedian 0.043ms\nCPU 0.032ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.43979199995192175,
            "unit": "ms",
            "extra": "Min 0.346ms\nMax 0.516ms\nMean 0.437ms\nStdDev 0.070ms\nMedian 0.440ms\nCPU 0.437ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.142625000047701,
            "unit": "ms",
            "extra": "Min 7.645ms\nMax 20.642ms\nMean 12.185ms\nStdDev 5.129ms\nMedian 11.143ms\nCPU 1.280ms"
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
          "id": "455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8",
          "message": "feat: add sentry_attachment_set_content_type() (#1276)\n\n* feat: add sentry_attachment_set_content_type()\n\n* drop content_type_owned\n\n* add _n",
          "timestamp": "2025-06-16T15:55:01+02:00",
          "tree_id": "0af59a742a894ffc9893e993308fbf3fa7f147b9",
          "url": "https://github.com/getsentry/sentry-native/commit/455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8"
        },
        "date": 1750082278066,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.446457999989661,
            "unit": "ms",
            "extra": "Min 3.087ms\nMax 4.826ms\nMean 4.147ms\nStdDev 0.776ms\nMedian 4.446ms\nCPU 2.267ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.963125000012724,
            "unit": "ms",
            "extra": "Min 3.087ms\nMax 5.737ms\nMean 4.274ms\nStdDev 1.109ms\nMedian 3.963ms\nCPU 2.309ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 13.371041999960198,
            "unit": "ms",
            "extra": "Min 12.674ms\nMax 23.789ms\nMean 15.580ms\nStdDev 4.670ms\nMedian 13.371ms\nCPU 4.715ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.026459000025624846,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.048ms\nMean 0.024ms\nStdDev 0.017ms\nMedian 0.026ms\nCPU 0.024ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.20749999998770363,
            "unit": "ms",
            "extra": "Min 0.186ms\nMax 0.225ms\nMean 0.206ms\nStdDev 0.017ms\nMedian 0.207ms\nCPU 0.206ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.120208999992883,
            "unit": "ms",
            "extra": "Min 6.081ms\nMax 12.042ms\nMean 8.173ms\nStdDev 2.341ms\nMedian 7.120ms\nCPU 0.899ms"
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
          "id": "99e598206296b63301919b9bc20e36c39adb51a1",
          "message": "chore: `breakpad` upstream update (#1277)\n\n* update breakpad + lss\n\n* CHANGELOG.md\n\n* breakpad post-merge",
          "timestamp": "2025-06-17T12:42:03+02:00",
          "tree_id": "3bc4293c0013fb354981e4896e27aa66ea889b16",
          "url": "https://github.com/getsentry/sentry-native/commit/99e598206296b63301919b9bc20e36c39adb51a1"
        },
        "date": 1750157113866,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.3051249999971333,
            "unit": "ms",
            "extra": "Min 3.086ms\nMax 7.672ms\nMean 4.960ms\nStdDev 2.427ms\nMedian 3.305ms\nCPU 2.822ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1786249999754546,
            "unit": "ms",
            "extra": "Min 2.847ms\nMax 5.508ms\nMean 3.856ms\nStdDev 1.140ms\nMedian 3.179ms\nCPU 2.297ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.980833999984952,
            "unit": "ms",
            "extra": "Min 10.917ms\nMax 16.154ms\nMean 12.855ms\nStdDev 2.064ms\nMedian 11.981ms\nCPU 4.089ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013875000036023266,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.105ms\nMean 0.035ms\nStdDev 0.041ms\nMedian 0.014ms\nCPU 0.034ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2120840000543467,
            "unit": "ms",
            "extra": "Min 0.205ms\nMax 0.231ms\nMean 0.214ms\nStdDev 0.010ms\nMedian 0.212ms\nCPU 0.213ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.601541999951223,
            "unit": "ms",
            "extra": "Min 5.244ms\nMax 8.950ms\nMean 7.518ms\nStdDev 1.712ms\nMedian 8.602ms\nCPU 0.786ms"
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
          "id": "6aaccb486304e891170fd7489c4d2cf76f8bfc1d",
          "message": "docs: remove `sentry_event_value_add_stacktrace()` from example (#1281)",
          "timestamp": "2025-06-20T15:16:54+02:00",
          "tree_id": "5270710c3fb3f6847aba3bd3bade5a46ca262e5d",
          "url": "https://github.com/getsentry/sentry-native/commit/6aaccb486304e891170fd7489c4d2cf76f8bfc1d"
        },
        "date": 1750425608592,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.903500000035365,
            "unit": "ms",
            "extra": "Min 3.693ms\nMax 6.864ms\nMean 4.540ms\nStdDev 1.339ms\nMedian 3.904ms\nCPU 2.702ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.6331659999859767,
            "unit": "ms",
            "extra": "Min 3.566ms\nMax 5.465ms\nMean 3.996ms\nStdDev 0.823ms\nMedian 3.633ms\nCPU 2.523ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.375334000035764,
            "unit": "ms",
            "extra": "Min 10.313ms\nMax 22.302ms\nMean 14.045ms\nStdDev 4.966ms\nMedian 11.375ms\nCPU 4.403ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.028874999998151907,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.037ms\nMean 0.023ms\nStdDev 0.013ms\nMedian 0.029ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2793329999803973,
            "unit": "ms",
            "extra": "Min 0.237ms\nMax 0.300ms\nMean 0.269ms\nStdDev 0.029ms\nMedian 0.279ms\nCPU 0.269ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.972499999984393,
            "unit": "ms",
            "extra": "Min 5.483ms\nMax 6.368ms\nMean 5.920ms\nStdDev 0.334ms\nMedian 5.972ms\nCPU 0.860ms"
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
          "id": "889d59aa2d165850e7fcdbedcb9770144638874f",
          "message": "chore: xbox compilation fixes and cleanup (#1284)\n\n* fix: xbox compilation\n\n* replace checks of _GAMING_XBOX_SCARLETT with SENTRY_PLATFORM_XBOX_SCARLETT\n\n* test: skip tests for missing features",
          "timestamp": "2025-06-24T14:52:27+02:00",
          "tree_id": "c6bb64d8f4f63b2374351a0a9ea016cf2782b76e",
          "url": "https://github.com/getsentry/sentry-native/commit/889d59aa2d165850e7fcdbedcb9770144638874f"
        },
        "date": 1750769776531,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.933708999999453,
            "unit": "ms",
            "extra": "Min 3.459ms\nMax 9.600ms\nMean 6.260ms\nStdDev 2.724ms\nMedian 4.934ms\nCPU 3.611ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 6.6647500000271975,
            "unit": "ms",
            "extra": "Min 3.921ms\nMax 12.473ms\nMean 7.008ms\nStdDev 3.467ms\nMedian 6.665ms\nCPU 3.888ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 27.98587500001304,
            "unit": "ms",
            "extra": "Min 18.530ms\nMax 49.416ms\nMean 32.271ms\nStdDev 13.732ms\nMedian 27.986ms\nCPU 9.539ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.018124999996871338,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.036ms\nMean 0.022ms\nStdDev 0.012ms\nMedian 0.018ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.29845799997474387,
            "unit": "ms",
            "extra": "Min 0.239ms\nMax 0.388ms\nMean 0.300ms\nStdDev 0.056ms\nMedian 0.298ms\nCPU 0.286ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.883332999971572,
            "unit": "ms",
            "extra": "Min 7.184ms\nMax 14.739ms\nMean 9.794ms\nStdDev 3.115ms\nMedian 8.883ms\nCPU 1.070ms"
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
          "id": "9dd7d6c61a1a3406fc613b4e2bad40baacc5453f",
          "message": "feat: support attaching bytes (#1275)\n\n* feat: Support attaching bytes\n\n* fix: move to_crashpad_attachment out of extern C\n\n> warning C4190: 'to_crashpad_attachment' has C-linkage specified,\n> but returns UDT 'crashpad::Attachment' which is incompatible with C\n\n* fix: lint\n\n* fix: unreachable\n\n* test: integration\n\n* test: rename\n\n* test: attaching bytes to crashpad is supported on win32 and linux\n\n* crashpad: dump byte attachments on disk\n\n* fix: windows\n\n* let crashpad ensure unique file names\n\n* fix sentry__attachment_from_buffer\n\n* clean up unused uuid\n\n* Update external/crashpad\n\n* alternative: ensure unique file in sentry_backend_crashpad\n\n* clean up\n\n* clean up more\n\n* switch to std::filesystem\n\n* fix leaks in backends\n\n* add sentry__attachment_from_path for convenience and to reduce diff\n\n* fix self-review findings\n\n* revert accidental ws changes\n\n* fix attachment_clone\n\ntype & content_type are passed separately and content_type is cloned in\nsentry__attachments_add()\n\n* unit-testable sentry__path_unique() to back the \"-N.tar.gz\" claims\n\n* include <string>\n\n* ref: unique paths for byte attachments\n\n* add note about unique file names with crashpad\n\n* add missing null checks for screenshots\n\n* attachment_clone: add missing error handling\n\n* add note and missing test for buffer attachment comparison\n\n* Bump external/crashpad\n\n* Update external/crashpad\n\n* attachment_eq: clarify with a comment\n\n* document behavior regarding duplicate attachments\n\n* sentry__attachments_remove: replace attachment_eq with ptr cmp",
          "timestamp": "2025-06-24T16:27:14+02:00",
          "tree_id": "9e05ee000fa10a5e67a901a9fb21081b48123122",
          "url": "https://github.com/getsentry/sentry-native/commit/9dd7d6c61a1a3406fc613b4e2bad40baacc5453f"
        },
        "date": 1750775500332,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.255125000042881,
            "unit": "ms",
            "extra": "Min 4.049ms\nMax 7.106ms\nMean 4.751ms\nStdDev 1.320ms\nMedian 4.255ms\nCPU 2.724ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.092708000006496,
            "unit": "ms",
            "extra": "Min 4.389ms\nMax 9.189ms\nMean 5.881ms\nStdDev 1.905ms\nMedian 5.093ms\nCPU 3.158ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 28.31025000000409,
            "unit": "ms",
            "extra": "Min 20.138ms\nMax 41.591ms\nMean 28.790ms\nStdDev 8.490ms\nMedian 28.310ms\nCPU 7.028ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.04058400003259521,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.042ms\nMean 0.033ms\nStdDev 0.014ms\nMedian 0.041ms\nCPU 0.032ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.26004099998999664,
            "unit": "ms",
            "extra": "Min 0.220ms\nMax 0.647ms\nMean 0.369ms\nStdDev 0.184ms\nMedian 0.260ms\nCPU 0.368ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.364083000013125,
            "unit": "ms",
            "extra": "Min 7.886ms\nMax 14.654ms\nMean 10.409ms\nStdDev 2.746ms\nMedian 10.364ms\nCPU 0.947ms"
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
          "id": "75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79",
          "message": "fix: xbox config typo (#1286)",
          "timestamp": "2025-06-24T20:48:09+02:00",
          "tree_id": "1b79f2f0327ba5e69ff5d101608f6752ebebb5b5",
          "url": "https://github.com/getsentry/sentry-native/commit/75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79"
        },
        "date": 1750791082737,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.741415999997116,
            "unit": "ms",
            "extra": "Min 4.287ms\nMax 14.659ms\nMean 9.089ms\nStdDev 4.673ms\nMedian 9.741ms\nCPU 5.018ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.855459000078554,
            "unit": "ms",
            "extra": "Min 3.330ms\nMax 4.188ms\nMean 3.773ms\nStdDev 0.412ms\nMedian 3.855ms\nCPU 2.192ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.299457999939477,
            "unit": "ms",
            "extra": "Min 9.921ms\nMax 17.137ms\nMean 12.213ms\nStdDev 2.829ms\nMedian 11.299ms\nCPU 4.413ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03016700009084161,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.067ms\nMean 0.032ms\nStdDev 0.024ms\nMedian 0.030ms\nCPU 0.031ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.22416700005578605,
            "unit": "ms",
            "extra": "Min 0.217ms\nMax 0.278ms\nMean 0.238ms\nStdDev 0.025ms\nMedian 0.224ms\nCPU 0.238ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.433042000002388,
            "unit": "ms",
            "extra": "Min 6.064ms\nMax 10.146ms\nMean 7.571ms\nStdDev 1.610ms\nMedian 7.433ms\nCPU 1.242ms"
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
          "id": "7646cac9c760c6734741d96320ec1e084b1e00f3",
          "message": "feat: add `sentry_attachment_set_filename()` (#1285)\n\n* feat: add `sentry_attachment_set_filename()`\n\n* add missing null check\n\n* crashpad: adapt ensure_unique_path",
          "timestamp": "2025-06-25T09:10:39+02:00",
          "tree_id": "6662413ce89811f88fc9b2b01f77313e006a6d87",
          "url": "https://github.com/getsentry/sentry-native/commit/7646cac9c760c6734741d96320ec1e084b1e00f3"
        },
        "date": 1750835588153,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.112000000101034,
            "unit": "ms",
            "extra": "Min 3.071ms\nMax 3.285ms\nMean 3.145ms\nStdDev 0.086ms\nMedian 3.112ms\nCPU 1.776ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.11658400005399,
            "unit": "ms",
            "extra": "Min 2.873ms\nMax 4.182ms\nMean 3.414ms\nStdDev 0.555ms\nMedian 3.117ms\nCPU 1.962ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.631416000023819,
            "unit": "ms",
            "extra": "Min 9.487ms\nMax 10.906ms\nMean 10.365ms\nStdDev 0.586ms\nMedian 10.631ms\nCPU 3.631ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03120799999578594,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.039ms\nMean 0.024ms\nStdDev 0.015ms\nMedian 0.031ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.18941599989830138,
            "unit": "ms",
            "extra": "Min 0.152ms\nMax 0.240ms\nMean 0.198ms\nStdDev 0.038ms\nMedian 0.189ms\nCPU 0.198ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.612292000023444,
            "unit": "ms",
            "extra": "Min 5.441ms\nMax 9.776ms\nMean 6.405ms\nStdDev 1.886ms\nMedian 5.612ms\nCPU 0.872ms"
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
          "id": "28fb3edd0ef3638059ab86d4f734f6cc5f0c9652",
          "message": "meta: identify Xbox as a separate SDK name (#1287)\n\n* Update CHANGELOG.md\n* move static sdk identification + versioning below platform defs in order to reuse the platform defs rather than external ones.",
          "timestamp": "2025-06-25T10:52:07+02:00",
          "tree_id": "30ce0a45de61b725825d2c3bbd76a94c01d5a8bd",
          "url": "https://github.com/getsentry/sentry-native/commit/28fb3edd0ef3638059ab86d4f734f6cc5f0c9652"
        },
        "date": 1750841726762,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.183457999999973,
            "unit": "ms",
            "extra": "Min 3.515ms\nMax 12.366ms\nMean 6.431ms\nStdDev 3.843ms\nMedian 4.183ms\nCPU 4.427ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.813707999995586,
            "unit": "ms",
            "extra": "Min 3.259ms\nMax 5.994ms\nMean 4.115ms\nStdDev 1.112ms\nMedian 3.814ms\nCPU 2.536ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 26.70508399995697,
            "unit": "ms",
            "extra": "Min 14.058ms\nMax 40.455ms\nMean 26.978ms\nStdDev 10.273ms\nMedian 26.705ms\nCPU 8.213ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.029290999975728482,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.084ms\nMean 0.033ms\nStdDev 0.030ms\nMedian 0.029ms\nCPU 0.032ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24358300004223565,
            "unit": "ms",
            "extra": "Min 0.204ms\nMax 0.272ms\nMean 0.241ms\nStdDev 0.026ms\nMedian 0.244ms\nCPU 0.241ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.670415999982197,
            "unit": "ms",
            "extra": "Min 7.409ms\nMax 17.207ms\nMean 11.090ms\nStdDev 3.683ms\nMedian 10.670ms\nCPU 1.236ms"
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
          "id": "e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112",
          "message": "chore: enable PS stack unwinding and module resolution (#1282)\n\n* chore: enable PS stack unwinding and module resolution\n\n* symbolize stacktrace automatically on PS",
          "timestamp": "2025-06-25T11:09:57+02:00",
          "tree_id": "f660999902c870f1ee163adc984d2245c84d258a",
          "url": "https://github.com/getsentry/sentry-native/commit/e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112"
        },
        "date": 1750842766330,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.277209000032144,
            "unit": "ms",
            "extra": "Min 2.969ms\nMax 4.002ms\nMean 3.329ms\nStdDev 0.419ms\nMedian 3.277ms\nCPU 2.031ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.7439160000189986,
            "unit": "ms",
            "extra": "Min 3.480ms\nMax 4.566ms\nMean 3.890ms\nStdDev 0.446ms\nMedian 3.744ms\nCPU 2.368ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.462458999977116,
            "unit": "ms",
            "extra": "Min 13.066ms\nMax 23.373ms\nMean 17.595ms\nStdDev 4.082ms\nMedian 18.462ms\nCPU 5.298ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.035625000009531504,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.084ms\nMean 0.047ms\nStdDev 0.031ms\nMedian 0.036ms\nCPU 0.045ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.26274999999031934,
            "unit": "ms",
            "extra": "Min 0.212ms\nMax 0.617ms\nMean 0.354ms\nStdDev 0.172ms\nMedian 0.263ms\nCPU 0.354ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.021207999978742,
            "unit": "ms",
            "extra": "Min 7.992ms\nMax 15.950ms\nMean 11.017ms\nStdDev 3.606ms\nMedian 9.021ms\nCPU 1.177ms"
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
          "id": "3f7bbd6193bda6814a7258da31971f21bf61da26",
          "message": "fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined (#1279)\n\n* fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined\n\n* run unit tests with custom path prefix\n\n* linter issue\n\n* fix cmake.py\n\n* Update tests/unit/sentry_testsupport.h",
          "timestamp": "2025-06-25T11:10:59+02:00",
          "tree_id": "be3a9dccafda19c04122b5283e8e33c4e4f818a2",
          "url": "https://github.com/getsentry/sentry-native/commit/3f7bbd6193bda6814a7258da31971f21bf61da26"
        },
        "date": 1750842830258,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.1663340000136486,
            "unit": "ms",
            "extra": "Min 3.052ms\nMax 3.843ms\nMean 3.319ms\nStdDev 0.331ms\nMedian 3.166ms\nCPU 1.952ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.4150829999930465,
            "unit": "ms",
            "extra": "Min 2.958ms\nMax 4.047ms\nMean 3.502ms\nStdDev 0.426ms\nMedian 3.415ms\nCPU 1.961ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.662917000023754,
            "unit": "ms",
            "extra": "Min 11.376ms\nMax 17.548ms\nMean 13.489ms\nStdDev 2.846ms\nMedian 11.663ms\nCPU 5.331ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009250000005067704,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.028ms\nMean 0.013ms\nStdDev 0.009ms\nMedian 0.009ms\nCPU 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24516600001334155,
            "unit": "ms",
            "extra": "Min 0.196ms\nMax 0.421ms\nMean 0.267ms\nStdDev 0.088ms\nMedian 0.245ms\nCPU 0.267ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.046916999968289,
            "unit": "ms",
            "extra": "Min 5.665ms\nMax 9.145ms\nMean 6.976ms\nStdDev 1.378ms\nMedian 7.047ms\nCPU 0.903ms"
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
          "id": "6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc",
          "message": "Merge branch 'release/0.9.1'",
          "timestamp": "2025-06-25T11:07:11Z",
          "tree_id": "51b4af13587545183a4c80ccce4bad6490ba3728",
          "url": "https://github.com/getsentry/sentry-native/commit/6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc"
        },
        "date": 1750849839157,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.189999999994143,
            "unit": "ms",
            "extra": "Min 4.133ms\nMax 4.447ms\nMean 4.244ms\nStdDev 0.129ms\nMedian 4.190ms\nCPU 2.338ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.4580830000077185,
            "unit": "ms",
            "extra": "Min 4.347ms\nMax 6.991ms\nMean 5.487ms\nStdDev 0.964ms\nMedian 5.458ms\nCPU 3.058ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 15.115042000047652,
            "unit": "ms",
            "extra": "Min 13.388ms\nMax 25.249ms\nMean 17.268ms\nStdDev 4.912ms\nMedian 15.115ms\nCPU 5.910ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010875000043597538,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.044ms\nMean 0.021ms\nStdDev 0.016ms\nMedian 0.011ms\nCPU 0.021ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2660409999180047,
            "unit": "ms",
            "extra": "Min 0.241ms\nMax 0.304ms\nMean 0.274ms\nStdDev 0.026ms\nMedian 0.266ms\nCPU 0.273ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.084166999992703,
            "unit": "ms",
            "extra": "Min 6.745ms\nMax 20.556ms\nMean 11.673ms\nStdDev 5.988ms\nMedian 8.084ms\nCPU 1.660ms"
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
          "id": "d3eef89221f127831ea58ac06797c00c96a1d63c",
          "message": "chore: add clangd .cache to .gitignore (#1291)",
          "timestamp": "2025-06-30T14:43:53+02:00",
          "tree_id": "92e0c2dbe387b8c88d769a05c4b5e6c6f248dff7",
          "url": "https://github.com/getsentry/sentry-native/commit/d3eef89221f127831ea58ac06797c00c96a1d63c"
        },
        "date": 1751287593363,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.138333000014427,
            "unit": "ms",
            "extra": "Min 3.486ms\nMax 14.348ms\nMean 6.093ms\nStdDev 4.628ms\nMedian 4.138ms\nCPU 3.088ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.329082999982802,
            "unit": "ms",
            "extra": "Min 3.067ms\nMax 4.588ms\nMean 3.529ms\nStdDev 0.606ms\nMedian 3.329ms\nCPU 2.073ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.706374999983836,
            "unit": "ms",
            "extra": "Min 10.028ms\nMax 11.263ms\nMean 10.629ms\nStdDev 0.484ms\nMedian 10.706ms\nCPU 3.795ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010540999994645972,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.040ms\nMean 0.020ms\nStdDev 0.016ms\nMedian 0.011ms\nCPU 0.020ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.1997499999788488,
            "unit": "ms",
            "extra": "Min 0.184ms\nMax 0.278ms\nMean 0.211ms\nStdDev 0.039ms\nMedian 0.200ms\nCPU 0.211ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.882040999949822,
            "unit": "ms",
            "extra": "Min 5.679ms\nMax 16.635ms\nMean 8.007ms\nStdDev 4.824ms\nMedian 5.882ms\nCPU 0.940ms"
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
          "id": "6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4",
          "message": "ci: update kcov (#1292)\n\n* pin kcov at 8afe9f29c58ef575877664c7ba209328233b70cc",
          "timestamp": "2025-06-30T16:23:35+02:00",
          "tree_id": "fe98eff320080abd8001df117dedf875aac1d839",
          "url": "https://github.com/getsentry/sentry-native/commit/6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4"
        },
        "date": 1751293606705,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.9268339999694035,
            "unit": "ms",
            "extra": "Min 3.515ms\nMax 5.323ms\nMean 4.100ms\nStdDev 0.707ms\nMedian 3.927ms\nCPU 2.085ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.193333000012899,
            "unit": "ms",
            "extra": "Min 3.018ms\nMax 4.924ms\nMean 3.621ms\nStdDev 0.799ms\nMedian 3.193ms\nCPU 2.144ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.169375000008586,
            "unit": "ms",
            "extra": "Min 10.592ms\nMax 13.521ms\nMean 11.498ms\nStdDev 1.193ms\nMedian 11.169ms\nCPU 3.953ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03708299993832043,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.040ms\nMean 0.031ms\nStdDev 0.013ms\nMedian 0.037ms\nCPU 0.031ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.19629200005510938,
            "unit": "ms",
            "extra": "Min 0.178ms\nMax 0.322ms\nMean 0.230ms\nStdDev 0.061ms\nMedian 0.196ms\nCPU 0.230ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.911916000059136,
            "unit": "ms",
            "extra": "Min 5.695ms\nMax 6.039ms\nMean 5.894ms\nStdDev 0.131ms\nMedian 5.912ms\nCPU 0.868ms"
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
          "id": "b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a",
          "message": "feat: add `sentry_clear_attachments()` (#1290)\n\n* feat: add `sentry_clear_attachments()`\n\n* crashpad: clear attachments in one go\n\n* Bump external/crashpad\n\n* Update CHANGELOG.md\n\n* revise remove_buffer_file()\n\n* add integration test\n\n* revert clear_attachments_func\n\nan integration test revealed that it was too effective clearing even\nbreadcrumb attachments\n\n* swap lock order",
          "timestamp": "2025-06-30T20:49:54+02:00",
          "tree_id": "6e201fe7cb9ba9838bcaf761a2f37e56a20695de",
          "url": "https://github.com/getsentry/sentry-native/commit/b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a"
        },
        "date": 1751309547178,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.01600000000235,
            "unit": "ms",
            "extra": "Min 2.944ms\nMax 3.385ms\nMean 3.136ms\nStdDev 0.202ms\nMedian 3.016ms\nCPU 1.799ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.0876659999989897,
            "unit": "ms",
            "extra": "Min 2.976ms\nMax 3.394ms\nMean 3.113ms\nStdDev 0.165ms\nMedian 3.088ms\nCPU 1.799ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.643583999979,
            "unit": "ms",
            "extra": "Min 10.271ms\nMax 11.978ms\nMean 10.983ms\nStdDev 0.740ms\nMedian 10.644ms\nCPU 3.932ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00895800002354008,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.036ms\nMean 0.019ms\nStdDev 0.014ms\nMedian 0.009ms\nCPU 0.018ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2371670000229642,
            "unit": "ms",
            "extra": "Min 0.212ms\nMax 0.255ms\nMean 0.237ms\nStdDev 0.016ms\nMedian 0.237ms\nCPU 0.236ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.685041999981877,
            "unit": "ms",
            "extra": "Min 5.471ms\nMax 6.819ms\nMean 5.848ms\nStdDev 0.553ms\nMedian 5.685ms\nCPU 0.807ms"
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
          "id": "3d9c118b7314f76c2350dfdd819e39feda033bf3",
          "message": "feat: add `sentry_regenerate_trace()` (#1293)\n\n* add generate_trace() + test\n\n* changelog\n\n* fix\n\n* fix memleak\n\n* remove const to allow sentry_free\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-01T11:14:09+02:00",
          "tree_id": "abfab1993ac4ca8f50dc33019aa6de9b251dd9ae",
          "url": "https://github.com/getsentry/sentry-native/commit/3d9c118b7314f76c2350dfdd819e39feda033bf3"
        },
        "date": 1751361419584,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.2889999999952124,
            "unit": "ms",
            "extra": "Min 2.974ms\nMax 3.563ms\nMean 3.306ms\nStdDev 0.217ms\nMedian 3.289ms\nCPU 1.875ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.5100420000162558,
            "unit": "ms",
            "extra": "Min 3.274ms\nMax 3.664ms\nMean 3.468ms\nStdDev 0.162ms\nMedian 3.510ms\nCPU 2.017ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.486332999917977,
            "unit": "ms",
            "extra": "Min 11.524ms\nMax 27.866ms\nMean 20.114ms\nStdDev 6.552ms\nMedian 19.486ms\nCPU 8.445ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.030916000014258316,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.036ms\nMean 0.024ms\nStdDev 0.012ms\nMedian 0.031ms\nCPU 0.024ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.27266600000075414,
            "unit": "ms",
            "extra": "Min 0.221ms\nMax 0.320ms\nMean 0.269ms\nStdDev 0.039ms\nMedian 0.273ms\nCPU 0.269ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.232040999952005,
            "unit": "ms",
            "extra": "Min 7.060ms\nMax 13.944ms\nMean 8.538ms\nStdDev 3.025ms\nMedian 7.232ms\nCPU 1.104ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bruno@brunogarcia.com",
            "name": "Bruno Garcia",
            "username": "bruno-garcia"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e0b3f8bba161a732463de2f32e00e89d154f7363",
          "message": "compiles on xbox one (#1294)\n\n* feat: xbox one support\n\n* define\n\n* wip\n\n* multiline command\n\n* it builds\n\n* gxdk_base_toolchain.cmake\n\n* update external/breakpad\n\n* fix'n'tweak xbox defines\n\n* Revert \"gxdk_base_toolchain.cmake\"\n\nThis reverts commit 18541aa8e3cada297d5212932dc1ec898ed5d5e6.\n\n* gxdk_toolchain.cmake - take 2\n\nhttps://github.com/microsoft/Xbox-GDK-Samples/blob/aa45b831e7a71160a69a7d13e9d74844dc6aa210/Samples/Tools/CMakeGDKExample/gxdk_toolchain.cmake\n\n* fix formatting\n\n* toolchain md\n\n* changelog\n\n* Update external/breakpad\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-03T16:28:39-04:00",
          "tree_id": "53f3f9cfc8842ce0d121144d90fddafb2b803f5c",
          "url": "https://github.com/getsentry/sentry-native/commit/e0b3f8bba161a732463de2f32e00e89d154f7363"
        },
        "date": 1751574669355,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.049790999966717,
            "unit": "ms",
            "extra": "Min 2.727ms\nMax 3.764ms\nMean 3.112ms\nStdDev 0.390ms\nMedian 3.050ms\nCPU 1.748ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.314292000027308,
            "unit": "ms",
            "extra": "Min 3.032ms\nMax 4.862ms\nMean 3.573ms\nStdDev 0.732ms\nMedian 3.314ms\nCPU 2.045ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.331457999996019,
            "unit": "ms",
            "extra": "Min 9.785ms\nMax 10.936ms\nMean 10.374ms\nStdDev 0.428ms\nMedian 10.331ms\nCPU 3.771ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.031040999942888448,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.033ms\nMean 0.025ms\nStdDev 0.011ms\nMedian 0.031ms\nCPU 0.025ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24425000003702735,
            "unit": "ms",
            "extra": "Min 0.177ms\nMax 0.380ms\nMean 0.252ms\nStdDev 0.078ms\nMedian 0.244ms\nCPU 0.252ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.948459000023831,
            "unit": "ms",
            "extra": "Min 5.546ms\nMax 7.963ms\nMean 6.248ms\nStdDev 0.975ms\nMedian 5.948ms\nCPU 0.902ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "rom4ek93@gmail.com",
            "name": "Roman Zavarnitsyn",
            "username": "romtsn"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "98b1bd883824ad6a63c2e6dcc2f07be407c73215",
          "message": "ci(publishing): Use shim ossrh-staging-api server to publish to the new central repo  (#1299)\n\n* Update settings.xml\n\n* Update .craft.yml",
          "timestamp": "2025-07-07T10:21:01+02:00",
          "tree_id": "f6021f5cba3e334cd66083b22d42914810c758ac",
          "url": "https://github.com/getsentry/sentry-native/commit/98b1bd883824ad6a63c2e6dcc2f07be407c73215"
        },
        "date": 1751876640544,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 4.506124999977601,
            "unit": "ms",
            "extra": "Min 3.781ms\nMax 9.004ms\nMean 5.318ms\nStdDev 2.094ms\nMedian 4.506ms\nCPU 2.696ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.864249999992353,
            "unit": "ms",
            "extra": "Min 3.231ms\nMax 13.772ms\nMean 5.707ms\nStdDev 4.518ms\nMedian 3.864ms\nCPU 2.751ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 12.853624999991098,
            "unit": "ms",
            "extra": "Min 11.529ms\nMax 29.157ms\nMean 16.148ms\nStdDev 7.388ms\nMedian 12.854ms\nCPU 5.034ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.031041999989156466,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.046ms\nMean 0.027ms\nStdDev 0.017ms\nMedian 0.031ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.27716599998939273,
            "unit": "ms",
            "extra": "Min 0.237ms\nMax 0.346ms\nMean 0.280ms\nStdDev 0.041ms\nMedian 0.277ms\nCPU 0.279ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.318041999975321,
            "unit": "ms",
            "extra": "Min 6.007ms\nMax 6.575ms\nMean 6.285ms\nStdDev 0.208ms\nMedian 6.318ms\nCPU 0.951ms"
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
          "id": "092202901f9b6fc16e94b7ce4ba2ac2b0afaee41",
          "message": "chore: update xbox toolchain and disable FS-dependent tests (#1302)\n\n* disable tests that require FS access\n\n* disable modulefinder test\n\n* chore: update xbox toolchains\n\n* fixup\n\n* try to reenable modulefinder\n\n* chore: changelog",
          "timestamp": "2025-07-08T18:10:43+02:00",
          "tree_id": "d5a97fdb483155e76f97d6a071eb0df625af382b",
          "url": "https://github.com/getsentry/sentry-native/commit/092202901f9b6fc16e94b7ce4ba2ac2b0afaee41"
        },
        "date": 1751991249978,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.6204170000219165,
            "unit": "ms",
            "extra": "Min 4.088ms\nMax 6.912ms\nMean 5.976ms\nStdDev 1.195ms\nMedian 6.620ms\nCPU 3.818ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.919125000014901,
            "unit": "ms",
            "extra": "Min 4.573ms\nMax 6.358ms\nMean 5.313ms\nStdDev 0.760ms\nMedian 4.919ms\nCPU 3.091ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.550166999995781,
            "unit": "ms",
            "extra": "Min 10.829ms\nMax 15.676ms\nMean 12.302ms\nStdDev 1.928ms\nMedian 11.550ms\nCPU 4.412ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.038874999972904334,
            "unit": "ms",
            "extra": "Min 0.016ms\nMax 0.199ms\nMean 0.066ms\nStdDev 0.075ms\nMedian 0.039ms\nCPU 0.066ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.24987499995177131,
            "unit": "ms",
            "extra": "Min 0.234ms\nMax 0.295ms\nMean 0.256ms\nStdDev 0.024ms\nMedian 0.250ms\nCPU 0.256ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.20429199998307,
            "unit": "ms",
            "extra": "Min 6.571ms\nMax 8.089ms\nMean 7.212ms\nStdDev 0.566ms\nMedian 7.204ms\nCPU 1.099ms"
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
          "id": "9ed9a70d71d98a1f38f8194334c9d1a40bcaed85",
          "message": "meta: add SENTRY_DEPRECATED(msg) macro (#1308)\n\n* meta: add SENTRY_DEPRECATED(msg) macro\n\n* fix deprecation warnings in tests\n\n* Update CHANGELOG.md\n\n* add version checks to be safe\n\n* try to make msvc happy\n\n* add SENTRY_TEST_DEPRECATED\n\n* Revert \"fix deprecation warnings in tests\"\n\nThis reverts commit 0245434bd1a37e3057111e6bb0272c23c2157963.\n\n* Use SENTRY_TEST_DEPRECATED\n\n* fix SENTRY_TEST_DEPRECATED formatting",
          "timestamp": "2025-07-10T18:15:12+02:00",
          "tree_id": "411d1040913b5a1fd7d66d55cd2fd3756c37ce12",
          "url": "https://github.com/getsentry/sentry-native/commit/9ed9a70d71d98a1f38f8194334c9d1a40bcaed85"
        },
        "date": 1752164263451,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.998959000024115,
            "unit": "ms",
            "extra": "Min 2.462ms\nMax 3.233ms\nMean 2.936ms\nStdDev 0.324ms\nMedian 2.999ms\nCPU 1.598ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.0870830000822025,
            "unit": "ms",
            "extra": "Min 3.003ms\nMax 3.259ms\nMean 3.114ms\nStdDev 0.095ms\nMedian 3.087ms\nCPU 1.773ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.968832999992628,
            "unit": "ms",
            "extra": "Min 10.309ms\nMax 26.495ms\nMean 13.957ms\nStdDev 7.018ms\nMedian 10.969ms\nCPU 4.306ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03458300000147574,
            "unit": "ms",
            "extra": "Min 0.032ms\nMax 0.036ms\nMean 0.034ms\nStdDev 0.002ms\nMedian 0.035ms\nCPU 0.034ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.27062499998464773,
            "unit": "ms",
            "extra": "Min 0.167ms\nMax 0.317ms\nMean 0.253ms\nStdDev 0.066ms\nMedian 0.271ms\nCPU 0.253ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.649790999996185,
            "unit": "ms",
            "extra": "Min 5.485ms\nMax 5.948ms\nMean 5.681ms\nStdDev 0.204ms\nMedian 5.650ms\nCPU 0.811ms"
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
          "id": "bffc57deb53808b26e11ef2683c05340195f8b0d",
          "message": "feat: implement the new User Feedback API (#1304)\n\n* feat: implement the new User Feedback API\n\nhttps://develop.sentry.dev/sdk/data-model/envelope-items/#user-feedback\n\n* Fix heap-use-after-free\n\n* Update CHANGELOG.md\n\n* remove unused line3_end\n\n* what if we reused the old API?\n\n* use internal id to fix event association in Sentry Web UI\n\n* restore integration test for legacy user report\n\n* convert a deprecated \"user report\" to a new \"user feedback\"\n\n* conversion: note in docs & log info message\n\n* let sentry__envelope_add_user_feedback take ownership\n\n* revert back to option 1\n\n* deprecate sentry_value_new_user_feedback() & sentry_capture_user_feedback()\n\n* fix remaining deprecation warnings\n\n* msvc",
          "timestamp": "2025-07-11T12:28:24+02:00",
          "tree_id": "09bfefa430d3e1795ffea5c12ce54d3a1bc21273",
          "url": "https://github.com/getsentry/sentry-native/commit/bffc57deb53808b26e11ef2683c05340195f8b0d"
        },
        "date": 1752230301890,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.1378749999930733,
            "unit": "ms",
            "extra": "Min 2.905ms\nMax 3.518ms\nMean 3.217ms\nStdDev 0.242ms\nMedian 3.138ms\nCPU 1.753ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.133000000048014,
            "unit": "ms",
            "extra": "Min 3.024ms\nMax 3.221ms\nMean 3.119ms\nStdDev 0.082ms\nMedian 3.133ms\nCPU 1.736ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.95041600001423,
            "unit": "ms",
            "extra": "Min 10.059ms\nMax 14.996ms\nMean 11.516ms\nStdDev 2.005ms\nMedian 10.950ms\nCPU 3.906ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03420899997763627,
            "unit": "ms",
            "extra": "Min 0.029ms\nMax 0.150ms\nMean 0.056ms\nStdDev 0.052ms\nMedian 0.034ms\nCPU 0.055ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.22916600005373766,
            "unit": "ms",
            "extra": "Min 0.165ms\nMax 0.256ms\nMean 0.226ms\nStdDev 0.036ms\nMedian 0.229ms\nCPU 0.226ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.121832999951948,
            "unit": "ms",
            "extra": "Min 5.640ms\nMax 7.104ms\nMean 6.220ms\nStdDev 0.556ms\nMedian 6.122ms\nCPU 0.900ms"
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
          "id": "fef7868244cd34c1a6d8650d3c594b0bf2a47314",
          "message": "test: migrate from deprecated `sentry_new_function_transport` (#1310)",
          "timestamp": "2025-07-11T15:15:44+02:00",
          "tree_id": "4ac1f0a0ba99d764784cf93c3294007a95774d7f",
          "url": "https://github.com/getsentry/sentry-native/commit/fef7868244cd34c1a6d8650d3c594b0bf2a47314"
        },
        "date": 1752239900102,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.927874999954838,
            "unit": "ms",
            "extra": "Min 2.870ms\nMax 3.022ms\nMean 2.938ms\nStdDev 0.068ms\nMedian 2.928ms\nCPU 1.624ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.424333999987539,
            "unit": "ms",
            "extra": "Min 3.216ms\nMax 5.463ms\nMean 3.805ms\nStdDev 0.933ms\nMedian 3.424ms\nCPU 2.068ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.3424579999446,
            "unit": "ms",
            "extra": "Min 9.806ms\nMax 13.545ms\nMean 11.424ms\nStdDev 1.473ms\nMedian 11.342ms\nCPU 4.319ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.030791000085628184,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.037ms\nMean 0.028ms\nStdDev 0.012ms\nMedian 0.031ms\nCPU 0.028ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.1878329999271955,
            "unit": "ms",
            "extra": "Min 0.172ms\nMax 0.331ms\nMean 0.240ms\nStdDev 0.081ms\nMedian 0.188ms\nCPU 0.239ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.325542000036876,
            "unit": "ms",
            "extra": "Min 5.286ms\nMax 6.479ms\nMean 5.567ms\nStdDev 0.514ms\nMedian 5.326ms\nCPU 0.820ms"
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
          "id": "cbaaf1e9590781327ff9eda35b01fefcddd669c0",
          "message": "feat: add dynamic sampling context (#1254)\n\n* initial DSC implementation\n\n* add `sample_rand`\n\n* fix `sample_rand` generation\n\n* fix the fix + add test\n\n* update org_id expected size\n\n* add test + changelog + sample_rand for traces_sampler\n\n* add test + changelog + sample_rand for traces_sampler + dsc for events\n\n* add org_id tests\n\n* fix CHANGELOG.md\n\n* cleanup\n\n* post-merge fix\n\n* post-merge fix v2\n\n* split up test\n\n* fix CHANGELOG.md\n\n* org_id handling update\n\n* asan overflow fix\n\n* add warnings for missing DSC fields\n\n* swap `sentry_set_trace` for `sentry_regenerate_trace`\n\n* add clarifying note\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-15T11:28:05+02:00",
          "tree_id": "5c65231fce6629c7190a21ad5faca8a1575b5042",
          "url": "https://github.com/getsentry/sentry-native/commit/cbaaf1e9590781327ff9eda35b01fefcddd669c0"
        },
        "date": 1752571833626,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.2358329999624402,
            "unit": "ms",
            "extra": "Min 2.815ms\nMax 4.027ms\nMean 3.317ms\nStdDev 0.447ms\nMedian 3.236ms\nCPU 1.873ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.1205830000544665,
            "unit": "ms",
            "extra": "Min 2.713ms\nMax 8.498ms\nMean 4.101ms\nStdDev 2.465ms\nMedian 3.121ms\nCPU 1.823ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.338333000049715,
            "unit": "ms",
            "extra": "Min 9.247ms\nMax 11.316ms\nMean 10.183ms\nStdDev 0.867ms\nMedian 10.338ms\nCPU 3.475ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03191700000115816,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.033ms\nMean 0.023ms\nStdDev 0.013ms\nMedian 0.032ms\nCPU 0.022ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.23849999990943616,
            "unit": "ms",
            "extra": "Min 0.171ms\nMax 0.259ms\nMean 0.229ms\nStdDev 0.034ms\nMedian 0.238ms\nCPU 0.229ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.214249999970889,
            "unit": "ms",
            "extra": "Min 5.618ms\nMax 6.716ms\nMean 6.100ms\nStdDev 0.452ms\nMedian 6.214ms\nCPU 0.899ms"
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
          "id": "68b918dfd331f218904739b90fe68dd54a1d93ba",
          "message": "test: fix SENTRY_TEST(path_directory) to clean up after itself (#1314)",
          "timestamp": "2025-07-15T13:02:21+02:00",
          "tree_id": "911f165201c3be7020da3d8635688367e6290b8e",
          "url": "https://github.com/getsentry/sentry-native/commit/68b918dfd331f218904739b90fe68dd54a1d93ba"
        },
        "date": 1752577527215,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.37149999995745,
            "unit": "ms",
            "extra": "Min 3.084ms\nMax 4.012ms\nMean 3.457ms\nStdDev 0.357ms\nMedian 3.371ms\nCPU 1.970ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.630916999985857,
            "unit": "ms",
            "extra": "Min 3.323ms\nMax 4.771ms\nMean 4.152ms\nStdDev 0.733ms\nMedian 4.631ms\nCPU 2.419ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 14.296374999958061,
            "unit": "ms",
            "extra": "Min 12.311ms\nMax 15.600ms\nMean 14.064ms\nStdDev 1.311ms\nMedian 14.296ms\nCPU 5.050ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03404200003842561,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.080ms\nMean 0.038ms\nStdDev 0.026ms\nMedian 0.034ms\nCPU 0.037ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2243330000055721,
            "unit": "ms",
            "extra": "Min 0.219ms\nMax 0.342ms\nMean 0.256ms\nStdDev 0.053ms\nMedian 0.224ms\nCPU 0.255ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.399374999977226,
            "unit": "ms",
            "extra": "Min 5.937ms\nMax 9.585ms\nMean 7.480ms\nStdDev 1.349ms\nMedian 7.399ms\nCPU 1.204ms"
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
          "id": "2b92ca838366ddc76a464e57795c7aaea2265d34",
          "message": "fix(xbox): propagate GDK version to MSBuild (#1312)\n\n* fix: propagate GDK version to MSBuild\n\n* Update CHANGELOG.md",
          "timestamp": "2025-07-15T16:38:52+02:00",
          "tree_id": "f20bb7a137cc3952ac937135be5bd26084682f7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2b92ca838366ddc76a464e57795c7aaea2265d34"
        },
        "date": 1752590646586,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 13.552084000025388,
            "unit": "ms",
            "extra": "Min 9.860ms\nMax 15.166ms\nMean 13.135ms\nStdDev 2.159ms\nMedian 13.552ms\nCPU 6.966ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.672791999993933,
            "unit": "ms",
            "extra": "Min 5.157ms\nMax 18.943ms\nMean 9.615ms\nStdDev 5.443ms\nMedian 7.673ms\nCPU 5.657ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 40.736792000075184,
            "unit": "ms",
            "extra": "Min 19.006ms\nMax 46.014ms\nMean 35.339ms\nStdDev 11.035ms\nMedian 40.737ms\nCPU 11.669ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03712499994890095,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.047ms\nMean 0.029ms\nStdDev 0.016ms\nMedian 0.037ms\nCPU 0.028ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3132920001007733,
            "unit": "ms",
            "extra": "Min 0.245ms\nMax 1.026ms\nMean 0.567ms\nStdDev 0.399ms\nMedian 0.313ms\nCPU 0.566ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 19.172749999938787,
            "unit": "ms",
            "extra": "Min 8.760ms\nMax 49.411ms\nMean 21.651ms\nStdDev 16.584ms\nMedian 19.173ms\nCPU 1.440ms"
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
          "id": "70f38a25655323f2e1e336a8771725c335c41319",
          "message": "ci: enable tsan + fix: rework breadcrumb ringbuffer (#1309)\n\n* adapt python test runner\n\n* fix lock-order inversion in `sentry_start_session()`\n\nThis will typically not hit because the inversion is relevant with the lock acquisition in `sentry_init()` and these two are typically not called from two different threads at the same time.\n\nHowever, consistent lock-order is such a basic sanity step and will guard us from future functions that also require scope- and options-locks.\n\n* properly synchronize `executed_after_shutdown` in `SENTRY_TEST(task_queue)`\n\n* move pre-init in sentry_init outside the options lock\n\n* move set_context outside the scope lock in set_trace so we can prevent unnecessary scope lock recursion\n\n* fix early exit in sentry__scope_get_span_or_transaction (only relevant for tests)\n\n* fix concurrent access to TEST_CHECK macro, which updates an unsynced global in the test-framework\n\n* minor format\n\n* extract the ringbuffer into a separate module\n\nthis allows us to keep state close and remove the need to retrieve max_breadcrumbs from the options after sentry_init(). Which in turn removes the need for nested locks.\n\n* add \"lock handling\" to the contributor docs\n\n* synchronize access to acutest state in the concurrency unit tests\n\n* limit SENTRY_WITH_SCOPE in attachment tests to the lines that actually need the scope\n\n* introduce tsan configs into the matrix\n\n* remove ringbuffer get_len definition\n\n* add tsan.supp + support in test runner\n\n* disable tsan in integration tests that involve DEADLYSIGNALS\n\n* add GCC based TSAN tests as a toolchain cross-check\n\n* clean up\n\n* revert Werror for GCC due to crashpad (not solving this here)\n\n* increase TSAN verbosity for the internal check that fails\n\n* disable all crashpad runs for tsan since it fails an internal check when forking the handler\n\n* format\n\n* re-enable all crashing tests in the http/stdout integration tests\n\nI falsely assumed an issue in the signal handler itself because the internal check triggered by crashpad_handler forking coincided with DEADLYSIGNAL.\n\nTurns out DEADLYSIGNAL is handled in TSAN like in ASAN, i.e. non-fatal (puh).\n\nThe culprit for crashpad is the fork during initialization, which breaks with TSAN invariants.\n\n* move ringbuffer tests in separate tu\n\n* missed cmake update\n\n* get rid of another unnecessary lock nesting\n\n* clone the DSC when prepping for the envelope header\n\n* clone the DSC when prepping for the envelope header (also in event path)\n\n* dramatically limit scope lock when prepping envelopes.\n\nAlso ensure that the cloned DSC is freed if we don't add it as a header.\n\n* remove unused imports\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-16T17:21:01+02:00",
          "tree_id": "2861324adb9817ab119e8c2571dda42436827416",
          "url": "https://github.com/getsentry/sentry-native/commit/70f38a25655323f2e1e336a8771725c335c41319"
        },
        "date": 1752679448756,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.4276250000857544,
            "unit": "ms",
            "extra": "Min 3.010ms\nMax 3.937ms\nMean 3.380ms\nStdDev 0.367ms\nMedian 3.428ms\nCPU 1.902ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.290707999961342,
            "unit": "ms",
            "extra": "Min 3.154ms\nMax 5.817ms\nMean 4.781ms\nStdDev 1.204ms\nMedian 5.291ms\nCPU 2.796ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.96454099994571,
            "unit": "ms",
            "extra": "Min 14.108ms\nMax 25.231ms\nMean 18.630ms\nStdDev 4.350ms\nMedian 18.965ms\nCPU 6.146ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01912499999434658,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.047ms\nMean 0.024ms\nStdDev 0.016ms\nMedian 0.019ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.23233299998537404,
            "unit": "ms",
            "extra": "Min 0.212ms\nMax 0.292ms\nMean 0.240ms\nStdDev 0.032ms\nMedian 0.232ms\nCPU 0.240ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.122541000081583,
            "unit": "ms",
            "extra": "Min 5.626ms\nMax 6.952ms\nMean 6.176ms\nStdDev 0.483ms\nMedian 6.123ms\nCPU 0.940ms"
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
          "id": "c3aa571d4eceffcaf4987cde769c92f2bc099753",
          "message": "feat: add `sentry__path_get_mtime()` (#1317)\n\n* feat: add `sentry__path_get_mtime()`\n\n* Apply suggestions from code review\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T16:03:41+02:00",
          "tree_id": "3baeb34b123904e7fdce41b8585a07d9d6e6d617",
          "url": "https://github.com/getsentry/sentry-native/commit/c3aa571d4eceffcaf4987cde769c92f2bc099753"
        },
        "date": 1753452351983,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.7910829999768794,
            "unit": "ms",
            "extra": "Min 2.570ms\nMax 2.923ms\nMean 2.747ms\nStdDev 0.141ms\nMedian 2.791ms\nCPU 1.539ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.725290999990193,
            "unit": "ms",
            "extra": "Min 2.653ms\nMax 2.842ms\nMean 2.725ms\nStdDev 0.077ms\nMedian 2.725ms\nCPU 1.540ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 8.662916999981007,
            "unit": "ms",
            "extra": "Min 8.351ms\nMax 8.822ms\nMean 8.618ms\nStdDev 0.192ms\nMedian 8.663ms\nCPU 3.090ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012540999989596457,
            "unit": "ms",
            "extra": "Min 0.007ms\nMax 0.015ms\nMean 0.011ms\nStdDev 0.004ms\nMedian 0.013ms\nCPU 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.16175000001794615,
            "unit": "ms",
            "extra": "Min 0.157ms\nMax 0.183ms\nMean 0.165ms\nStdDev 0.011ms\nMedian 0.162ms\nCPU 0.164ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 4.3764590000137105,
            "unit": "ms",
            "extra": "Min 4.305ms\nMax 4.554ms\nMean 4.404ms\nStdDev 0.093ms\nMedian 4.376ms\nCPU 0.589ms"
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
          "id": "205831fda2ece8c784259f7abac6d297a65474a6",
          "message": "ref: crashpad: pre-generate crash event ID (#1319)\n\n* ref: crashpad: generate event ID on the client side\n\n* Update CHANGELOG.md\n\n* Update src/sentry_value.c\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* fixup\n\n* Update src/sentry_value.h\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T17:30:53+02:00",
          "tree_id": "2ae22807fa39861d2995772f90de749f8f52b9b0",
          "url": "https://github.com/getsentry/sentry-native/commit/205831fda2ece8c784259f7abac6d297a65474a6"
        },
        "date": 1753457671459,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 5.17233400000805,
            "unit": "ms",
            "extra": "Min 3.286ms\nMax 9.507ms\nMean 5.659ms\nStdDev 2.488ms\nMedian 5.172ms\nCPU 3.559ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.1996249999983775,
            "unit": "ms",
            "extra": "Min 3.209ms\nMax 8.121ms\nMean 6.564ms\nStdDev 1.936ms\nMedian 7.200ms\nCPU 3.920ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 14.346959000022252,
            "unit": "ms",
            "extra": "Min 10.106ms\nMax 21.804ms\nMean 14.364ms\nStdDev 4.692ms\nMedian 14.347ms\nCPU 5.944ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03183399996942171,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.035ms\nMean 0.027ms\nStdDev 0.010ms\nMedian 0.032ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2396250000629152,
            "unit": "ms",
            "extra": "Min 0.212ms\nMax 0.387ms\nMean 0.269ms\nStdDev 0.069ms\nMedian 0.240ms\nCPU 0.269ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.8154590000185635,
            "unit": "ms",
            "extra": "Min 5.664ms\nMax 7.074ms\nMean 6.020ms\nStdDev 0.593ms\nMedian 5.815ms\nCPU 0.892ms"
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
          "id": "1939e5d2b1636fce6320208a451b51a41aadcc47",
          "message": "test: suppress valgrind for `SENTRY_TEST(task_queue)` (#1323)",
          "timestamp": "2025-07-26T08:43:59-04:00",
          "tree_id": "f780ac08a01bbf8976d4039c54192f7f6d697977",
          "url": "https://github.com/getsentry/sentry-native/commit/1939e5d2b1636fce6320208a451b51a41aadcc47"
        },
        "date": 1753534031313,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 5.152916000042751,
            "unit": "ms",
            "extra": "Min 4.137ms\nMax 8.372ms\nMean 5.503ms\nStdDev 1.671ms\nMedian 5.153ms\nCPU 2.699ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.6595420001267485,
            "unit": "ms",
            "extra": "Min 4.498ms\nMax 8.351ms\nMean 5.559ms\nStdDev 1.649ms\nMedian 4.660ms\nCPU 3.273ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 14.01375000000371,
            "unit": "ms",
            "extra": "Min 13.112ms\nMax 18.621ms\nMean 15.301ms\nStdDev 2.374ms\nMedian 14.014ms\nCPU 6.706ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03424999999879219,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.046ms\nMean 0.033ms\nStdDev 0.013ms\nMedian 0.034ms\nCPU 0.033ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.26312499994674,
            "unit": "ms",
            "extra": "Min 0.246ms\nMax 0.329ms\nMean 0.272ms\nStdDev 0.033ms\nMedian 0.263ms\nCPU 0.271ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.074832999933278,
            "unit": "ms",
            "extra": "Min 6.114ms\nMax 12.120ms\nMean 7.910ms\nStdDev 2.462ms\nMedian 7.075ms\nCPU 1.287ms"
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
          "id": "2ac959eff4f76935fcdd4c81dd1cca1056c01f77",
          "message": "feat: read and capture envelopes (#1320)\n\n* feat: raw envelope headers\n\n* Update CHANGELOG.md\n\n* move parsing to sentry_envelope_read_from_file()\n\n* cast\n\n* feat: parse whole envelope\n\n* Fix ClangCL warnings\n\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(765,53): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(786,62): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(803,61): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\n\n* revert unnecessary changes\n\n* Wide-string variant for Windows\n\n* Improve tests\n\n* size_t payload_len\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* sentry_envelope_deserialize + tests\n\n* validate headers\n\n* don't advance ptr past the buffer even if it's never used\n\nmight make cursor satisfied?\n\n* sentry_capture_envelope: add extra null check\n\n* sentry_envelope_get_header_n: add null check\n\n* prevent overflow\n\n* add TODO comments for sentry_value_as_uint64 + SIZE_MAX\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>",
          "timestamp": "2025-07-29T07:54:02+02:00",
          "tree_id": "358171b00995b08d09eb24f60a2f2080cb3bbcb8",
          "url": "https://github.com/getsentry/sentry-native/commit/2ac959eff4f76935fcdd4c81dd1cca1056c01f77"
        },
        "date": 1753768611344,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.838791000021047,
            "unit": "ms",
            "extra": "Min 2.888ms\nMax 4.370ms\nMean 3.790ms\nStdDev 0.580ms\nMedian 3.839ms\nCPU 2.184ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 5.359583000085877,
            "unit": "ms",
            "extra": "Min 3.723ms\nMax 9.474ms\nMean 6.552ms\nStdDev 2.649ms\nMedian 5.360ms\nCPU 3.319ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.092624999946565,
            "unit": "ms",
            "extra": "Min 10.710ms\nMax 12.141ms\nMean 11.240ms\nStdDev 0.572ms\nMedian 11.093ms\nCPU 3.943ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03145899995615764,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.035ms\nMean 0.028ms\nStdDev 0.011ms\nMedian 0.031ms\nCPU 0.028ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.16083299999536393,
            "unit": "ms",
            "extra": "Min 0.154ms\nMax 0.193ms\nMean 0.166ms\nStdDev 0.016ms\nMedian 0.161ms\nCPU 0.166ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.671500000062224,
            "unit": "ms",
            "extra": "Min 5.358ms\nMax 6.719ms\nMean 5.838ms\nStdDev 0.523ms\nMedian 5.672ms\nCPU 0.822ms"
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
          "id": "93d69f4172c530d8e4ac581df2e80d97e24c7398",
          "message": "feat: add `sentry__process_spawn()` (#1318)\n\n* feat: add `sentry__process_spawn()`\n\n* stabilize SENTRY_TEST(process_spawn)\n\n* add null checks for executable->path\n\n* test: drop sentry__process_spawn return value check\n\njust to see if the CI/Valgrind is happy with the rest\n\n* sentry__process_spawn: remove falsy return value\n\nas claude put it\n\n> This is inherent to fully detached processes - you can't reliably detect launch success without IPC\n\n* avoid path lookup for security reasons\n\n* add note that arguments are not sanitized\n\n* windows: quote args if necessary",
          "timestamp": "2025-07-30T13:30:36+02:00",
          "tree_id": "6593bb13efcc69f02e077905dd2d47340e30e417",
          "url": "https://github.com/getsentry/sentry-native/commit/93d69f4172c530d8e4ac581df2e80d97e24c7398"
        },
        "date": 1753875176357,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.042791999973815,
            "unit": "ms",
            "extra": "Min 2.751ms\nMax 3.290ms\nMean 3.038ms\nStdDev 0.199ms\nMedian 3.043ms\nCPU 1.691ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.3692499999915526,
            "unit": "ms",
            "extra": "Min 2.944ms\nMax 4.572ms\nMean 3.526ms\nStdDev 0.634ms\nMedian 3.369ms\nCPU 1.993ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 9.2920409999806,
            "unit": "ms",
            "extra": "Min 9.020ms\nMax 11.587ms\nMean 9.747ms\nStdDev 1.074ms\nMedian 9.292ms\nCPU 3.497ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.018834000002243556,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.025ms\nMean 0.019ms\nStdDev 0.006ms\nMedian 0.019ms\nCPU 0.018ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2031669999951191,
            "unit": "ms",
            "extra": "Min 0.146ms\nMax 0.296ms\nMean 0.206ms\nStdDev 0.061ms\nMedian 0.203ms\nCPU 0.206ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.217791999939436,
            "unit": "ms",
            "extra": "Min 4.837ms\nMax 5.916ms\nMean 5.288ms\nStdDev 0.398ms\nMedian 5.218ms\nCPU 0.724ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "markus.hintersteiner@sentry.io",
            "name": "Markus Hintersteiner",
            "username": "markushi"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e2c2c0523ba9d14dac0c969216594c975a2a7709",
          "message": "fix(ci): Fix missing debug symbol upload for craft (#1327)\n\nThe `libsentry.so` and `libsentry-android.so` files where not uploaded to the git release sha, causing craft to be unable to find and upload these symbols to our symbol server.\n\n#skip-changelog",
          "timestamp": "2025-07-31T08:54:57+02:00",
          "tree_id": "a844f7ca37cb51840d220a0fbee724d848064acf",
          "url": "https://github.com/getsentry/sentry-native/commit/e2c2c0523ba9d14dac0c969216594c975a2a7709"
        },
        "date": 1753945077992,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.317416999948364,
            "unit": "ms",
            "extra": "Min 3.174ms\nMax 4.171ms\nMean 3.518ms\nStdDev 0.420ms\nMedian 3.317ms\nCPU 1.847ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.2842499999787833,
            "unit": "ms",
            "extra": "Min 2.757ms\nMax 3.900ms\nMean 3.259ms\nStdDev 0.426ms\nMedian 3.284ms\nCPU 1.861ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.554665999961799,
            "unit": "ms",
            "extra": "Min 10.091ms\nMax 11.257ms\nMean 10.549ms\nStdDev 0.460ms\nMedian 10.555ms\nCPU 3.809ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03170800005136698,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.034ms\nMean 0.023ms\nStdDev 0.013ms\nMedian 0.032ms\nCPU 0.024ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.21008399994570937,
            "unit": "ms",
            "extra": "Min 0.189ms\nMax 0.221ms\nMean 0.207ms\nStdDev 0.013ms\nMedian 0.210ms\nCPU 0.207ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.819999999857828,
            "unit": "ms",
            "extra": "Min 5.550ms\nMax 5.984ms\nMean 5.777ms\nStdDev 0.165ms\nMedian 5.820ms\nCPU 0.848ms"
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
          "id": "98b38912bc3ed32e4b014cac72cd8f2aff497608",
          "message": "feat: add `(u)int64 sentry_value_t` type (#1326)\n\n* add (u)int64 sentry_value_t type\n\n* add value_to_msgpack missing switch cases\n\n* remove undefined behavior test (C99 6.3.1.4)\n\n* avoid Windows sized integer name collision\n\n* cleanup & apply code review feedback\n\n* more cleanup & remove type coercion\n\n* correctly parse (u)int64 in sentry__value_from_json\n\n* CHANGELOG.md\n\n* always send uint64 as string\n\n* simplify value_from_json integer parsing\n\n* use stdint macros instead of literals\n\n* fixed value_stringify + added unit test\n\n* Apply suggestions from code review\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>\n\n* free stringify memory + cleanup test\n\n* stringify NAN / INFINITY test with contains check\n\n* add explanation/link for writing uint64 as string\n\n* Fix macro not using parameter\n\n* update ndk gradle dependencies\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-31T12:37:10+02:00",
          "tree_id": "783406732616d145a8f2eb92a5a95fc86cfd6c7f",
          "url": "https://github.com/getsentry/sentry-native/commit/98b38912bc3ed32e4b014cac72cd8f2aff497608"
        },
        "date": 1753958423983,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 2.904666999938854,
            "unit": "ms",
            "extra": "Min 2.635ms\nMax 3.073ms\nMean 2.898ms\nStdDev 0.164ms\nMedian 2.905ms\nCPU 1.618ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.7857920000542435,
            "unit": "ms",
            "extra": "Min 2.755ms\nMax 2.820ms\nMean 2.784ms\nStdDev 0.027ms\nMedian 2.786ms\nCPU 1.587ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 8.402832999991006,
            "unit": "ms",
            "extra": "Min 8.205ms\nMax 9.148ms\nMean 8.524ms\nStdDev 0.373ms\nMedian 8.403ms\nCPU 3.060ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.006541999937326182,
            "unit": "ms",
            "extra": "Min 0.006ms\nMax 0.012ms\nMean 0.008ms\nStdDev 0.003ms\nMedian 0.007ms\nCPU 0.008ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.15954200000578567,
            "unit": "ms",
            "extra": "Min 0.120ms\nMax 0.165ms\nMean 0.147ms\nStdDev 0.021ms\nMedian 0.160ms\nCPU 0.147ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 4.589541999962421,
            "unit": "ms",
            "extra": "Min 4.546ms\nMax 5.097ms\nMean 4.712ms\nStdDev 0.230ms\nMedian 4.590ms\nCPU 0.674ms"
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
          "id": "79bf2dbc59b2cf149913f819deb4777707e193cd",
          "message": "ref: use SIZE_MAX in sentry_envelope_deserialize (#1328)\n\n* ref: use SIZE_MAX in sentry_envelope_deserialize\n\n* restore payload_len < 0 check\n\n* fix length parsing vuln\n\n* Update src/sentry_envelope.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-01T13:01:26+02:00",
          "tree_id": "fa61a69295ded8986cd7509f2eed29058bb5cb08",
          "url": "https://github.com/getsentry/sentry-native/commit/79bf2dbc59b2cf149913f819deb4777707e193cd"
        },
        "date": 1754046270600,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.7672500000098808,
            "unit": "ms",
            "extra": "Min 3.642ms\nMax 5.901ms\nMean 4.266ms\nStdDev 0.948ms\nMedian 3.767ms\nCPU 2.366ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.9207500001339213,
            "unit": "ms",
            "extra": "Min 3.769ms\nMax 7.179ms\nMean 4.548ms\nStdDev 1.475ms\nMedian 3.921ms\nCPU 2.786ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 14.201082999989012,
            "unit": "ms",
            "extra": "Min 13.185ms\nMax 19.945ms\nMean 15.804ms\nStdDev 3.081ms\nMedian 14.201ms\nCPU 5.567ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03558299999895098,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.112ms\nMean 0.047ms\nStdDev 0.038ms\nMedian 0.036ms\nCPU 0.045ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2494579998710833,
            "unit": "ms",
            "extra": "Min 0.240ms\nMax 0.304ms\nMean 0.266ms\nStdDev 0.029ms\nMedian 0.249ms\nCPU 0.266ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.3604169999725855,
            "unit": "ms",
            "extra": "Min 6.230ms\nMax 18.022ms\nMean 9.390ms\nStdDev 4.906ms\nMedian 7.360ms\nCPU 1.243ms"
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
          "id": "aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11",
          "message": "fix: add wildcard entries to valgrind suppression for pthread stack leak (#1330)",
          "timestamp": "2025-08-05T10:02:56+02:00",
          "tree_id": "36e09335b55bdfbcb825b1513e776d9972ae5283",
          "url": "https://github.com/getsentry/sentry-native/commit/aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11"
        },
        "date": 1754381187159,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.3111249999819847,
            "unit": "ms",
            "extra": "Min 3.106ms\nMax 4.062ms\nMean 3.431ms\nStdDev 0.369ms\nMedian 3.311ms\nCPU 1.958ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 2.8661670000929007,
            "unit": "ms",
            "extra": "Min 2.811ms\nMax 2.960ms\nMean 2.878ms\nStdDev 0.065ms\nMedian 2.866ms\nCPU 1.626ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 9.81279099994481,
            "unit": "ms",
            "extra": "Min 8.824ms\nMax 12.482ms\nMean 10.157ms\nStdDev 1.380ms\nMedian 9.813ms\nCPU 3.883ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.02537499995014514,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.030ms\nMean 0.023ms\nStdDev 0.008ms\nMedian 0.025ms\nCPU 0.023ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.14491699994323426,
            "unit": "ms",
            "extra": "Min 0.137ms\nMax 0.157ms\nMean 0.146ms\nStdDev 0.008ms\nMedian 0.145ms\nCPU 0.146ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.002041000011559,
            "unit": "ms",
            "extra": "Min 4.917ms\nMax 5.085ms\nMean 4.999ms\nStdDev 0.062ms\nMedian 5.002ms\nCPU 0.698ms"
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
          "id": "32e09d06b5184e562b8f2762400bf3e9e5e806f3",
          "message": "chore: Change issue template type to label (#1332)\n\n* Update issue template\n\nWe no longer use Issue Type, but Labels instead.\n\n* Update .github/ISSUE_TEMPLATE/bug_report.md\n\n---------\n\nCo-authored-by: Karl Heinz Struggl <kahest@users.noreply.github.com>",
          "timestamp": "2025-08-07T10:29:17+02:00",
          "tree_id": "ac35ea7b9bca448a239326161fcc223f8bce0c32",
          "url": "https://github.com/getsentry/sentry-native/commit/32e09d06b5184e562b8f2762400bf3e9e5e806f3"
        },
        "date": 1754555603270,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.46691700002566,
            "unit": "ms",
            "extra": "Min 2.958ms\nMax 3.832ms\nMean 3.461ms\nStdDev 0.319ms\nMedian 3.467ms\nCPU 1.993ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.080000000011296,
            "unit": "ms",
            "extra": "Min 2.786ms\nMax 3.696ms\nMean 3.128ms\nStdDev 0.358ms\nMedian 3.080ms\nCPU 1.751ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 10.358458999974118,
            "unit": "ms",
            "extra": "Min 9.540ms\nMax 11.927ms\nMean 10.455ms\nStdDev 0.902ms\nMedian 10.358ms\nCPU 3.851ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013000000024021574,
            "unit": "ms",
            "extra": "Min 0.008ms\nMax 0.037ms\nMean 0.019ms\nStdDev 0.013ms\nMedian 0.013ms\nCPU 0.018ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.21091700000397395,
            "unit": "ms",
            "extra": "Min 0.160ms\nMax 0.264ms\nMean 0.216ms\nStdDev 0.040ms\nMedian 0.211ms\nCPU 0.216ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 5.767832999993061,
            "unit": "ms",
            "extra": "Min 5.446ms\nMax 6.022ms\nMean 5.706ms\nStdDev 0.242ms\nMedian 5.768ms\nCPU 0.884ms"
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
          "id": "305dec3449cf43975d7987510382f7149c435115",
          "message": "chore: move xbox toolchain to sentry-xbox repository (#1329)\n\n* chore: move xbox toolchain to sentry-xbox repository\n\n* update platform support docs\n\n* chore: changelog\n\n* Update CHANGELOG.md\n\n* cleanup\n\n* keep sentry.native.xbox as sdk name\n\n* restore xbox_build.md\n\n* fixup: restore sentry.h properly",
          "timestamp": "2025-08-07T12:47:11+02:00",
          "tree_id": "80b06a68781990309fa25116c73e2f3092a1aee8",
          "url": "https://github.com/getsentry/sentry-native/commit/305dec3449cf43975d7987510382f7149c435115"
        },
        "date": 1754563839298,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 5.4161670000212325,
            "unit": "ms",
            "extra": "Min 4.923ms\nMax 11.362ms\nMean 6.534ms\nStdDev 2.726ms\nMedian 5.416ms\nCPU 3.351ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 4.856917000097383,
            "unit": "ms",
            "extra": "Min 3.407ms\nMax 6.438ms\nMean 4.709ms\nStdDev 1.278ms\nMedian 4.857ms\nCPU 2.687ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.95091700005014,
            "unit": "ms",
            "extra": "Min 12.441ms\nMax 27.285ms\nMean 20.233ms\nStdDev 6.961ms\nMedian 20.951ms\nCPU 6.807ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03158300000905001,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.048ms\nMean 0.028ms\nStdDev 0.018ms\nMedian 0.032ms\nCPU 0.027ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.29025000003457535,
            "unit": "ms",
            "extra": "Min 0.238ms\nMax 0.419ms\nMean 0.310ms\nStdDev 0.071ms\nMedian 0.290ms\nCPU 0.310ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.312582999929873,
            "unit": "ms",
            "extra": "Min 5.986ms\nMax 10.903ms\nMean 7.866ms\nStdDev 1.831ms\nMedian 7.313ms\nCPU 1.037ms"
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
          "id": "6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee",
          "message": "feat: manage trace life-cycle (#1270)\n\n* feat: manage trace duration\n\n* format after web-editor merge\n\n* clean up regenerate_trace() test after merge\n\n* prevent warning-as-error\n\n* add changelog\n\n* ensure trace_managed is turned off when regenerating a trace\n\n* update breaking changes\n\n* test that transaction no longer act as trace boundaries after a user regenerates a trace\n\n* highlight the topic of automatic trace boundaries in the docs for `sentry_set_trace()` and `sentry_regenerate_trace()`\n\n* Update tests/unit/test_tracing.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* remove duplicate includes in sentry_core.c\n\n* tighten regenerate part of the test against the trace generated during the previous transaction\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-08T12:24:46+02:00",
          "tree_id": "13cad9add2e8e83d579991f58deca96160144236",
          "url": "https://github.com/getsentry/sentry-native/commit/6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee"
        },
        "date": 1754648860503,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.036916000041856,
            "unit": "ms",
            "extra": "Min 2.791ms\nMax 3.379ms\nMean 3.026ms\nStdDev 0.226ms\nMedian 3.037ms\nCPU 1.640ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.4116670000230442,
            "unit": "ms",
            "extra": "Min 3.074ms\nMax 3.852ms\nMean 3.415ms\nStdDev 0.304ms\nMedian 3.412ms\nCPU 1.839ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 11.526375000016742,
            "unit": "ms",
            "extra": "Min 9.584ms\nMax 14.017ms\nMean 11.964ms\nStdDev 1.801ms\nMedian 11.526ms\nCPU 4.523ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.03254099999594473,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.034ms\nMean 0.028ms\nStdDev 0.010ms\nMedian 0.033ms\nCPU 0.028ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2317500000117434,
            "unit": "ms",
            "extra": "Min 0.202ms\nMax 0.333ms\nMean 0.246ms\nStdDev 0.053ms\nMedian 0.232ms\nCPU 0.245ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 6.627041999990979,
            "unit": "ms",
            "extra": "Min 6.506ms\nMax 10.481ms\nMean 7.401ms\nStdDev 1.724ms\nMedian 6.627ms\nCPU 1.033ms"
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
          "id": "d5c34db05c6d8cca302c2dface5063b0488df5be",
          "message": "craft: only include zip artifacts for github target (#1333)\n\n#skip-changelog",
          "timestamp": "2025-08-08T13:53:16+02:00",
          "tree_id": "bbe859661565e9a10a808263d75f3a5c6cf8aecf",
          "url": "https://github.com/getsentry/sentry-native/commit/d5c34db05c6d8cca302c2dface5063b0488df5be"
        },
        "date": 1754654335757,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 3.2179999999470965,
            "unit": "ms",
            "extra": "Min 2.980ms\nMax 3.594ms\nMean 3.244ms\nStdDev 0.247ms\nMedian 3.218ms\nCPU 1.757ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 3.4586670000180675,
            "unit": "ms",
            "extra": "Min 3.061ms\nMax 3.972ms\nMean 3.492ms\nStdDev 0.356ms\nMedian 3.459ms\nCPU 1.989ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.95116700006838,
            "unit": "ms",
            "extra": "Min 15.014ms\nMax 26.259ms\nMean 19.047ms\nStdDev 4.422ms\nMedian 18.951ms\nCPU 6.515ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.039750000041749445,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.051ms\nMean 0.031ms\nStdDev 0.020ms\nMedian 0.040ms\nCPU 0.031ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30820800009223603,
            "unit": "ms",
            "extra": "Min 0.229ms\nMax 0.628ms\nMean 0.359ms\nStdDev 0.155ms\nMedian 0.308ms\nCPU 0.359ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 7.366333000049963,
            "unit": "ms",
            "extra": "Min 6.431ms\nMax 15.499ms\nMean 8.840ms\nStdDev 3.795ms\nMedian 7.366ms\nCPU 0.996ms"
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
        "date": 1747311434045,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.814800000029209,
            "unit": "ms",
            "extra": "Min 6.749ms\nMax 6.979ms\nMean 6.830ms\nStdDev 0.088ms\nMedian 6.815ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.445399999937763,
            "unit": "ms",
            "extra": "Min 7.194ms\nMax 8.942ms\nMean 7.698ms\nStdDev 0.727ms\nMedian 7.445ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.185699999913595,
            "unit": "ms",
            "extra": "Min 16.901ms\nMax 17.481ms\nMean 17.220ms\nStdDev 0.239ms\nMedian 17.186ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009299999987888441,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.009ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2925999999661144,
            "unit": "ms",
            "extra": "Min 0.288ms\nMax 0.317ms\nMean 0.297ms\nStdDev 0.011ms\nMedian 0.293ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.29180000002816,
            "unit": "ms",
            "extra": "Min 8.922ms\nMax 9.328ms\nMean 9.203ms\nStdDev 0.169ms\nMedian 9.292ms"
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
          "id": "426c60558d68ac13204d9c9bfb64f5284cb36d7e",
          "message": "docs: windows test and formatter runners (#1247)",
          "timestamp": "2025-05-15T17:27:13+02:00",
          "tree_id": "7a79c6dc37e714afa484243ab15e43e50d3a99fb",
          "url": "https://github.com/getsentry/sentry-native/commit/426c60558d68ac13204d9c9bfb64f5284cb36d7e"
        },
        "date": 1747323121391,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.422799999972085,
            "unit": "ms",
            "extra": "Min 7.278ms\nMax 7.699ms\nMean 7.441ms\nStdDev 0.174ms\nMedian 7.423ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.2243000000753455,
            "unit": "ms",
            "extra": "Min 7.067ms\nMax 8.178ms\nMean 7.445ms\nStdDev 0.453ms\nMedian 7.224ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.473599999969338,
            "unit": "ms",
            "extra": "Min 18.735ms\nMax 20.278ms\nMean 19.538ms\nStdDev 0.700ms\nMedian 19.474ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011999999969702912,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.33800000005612674,
            "unit": "ms",
            "extra": "Min 0.304ms\nMax 0.367ms\nMean 0.337ms\nStdDev 0.027ms\nMedian 0.338ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.075900000060756,
            "unit": "ms",
            "extra": "Min 9.680ms\nMax 10.416ms\nMean 10.008ms\nStdDev 0.299ms\nMedian 10.076ms"
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
          "id": "c653de396546b870f563f320f15362282a2432b5",
          "message": "chore: update gradle scripts to current AGP/SDK usage (#1256)",
          "timestamp": "2025-05-27T15:08:13+02:00",
          "tree_id": "f3df760bb7c0e2f55580d7011626ef29a7b77cff",
          "url": "https://github.com/getsentry/sentry-native/commit/c653de396546b870f563f320f15362282a2432b5"
        },
        "date": 1748351534581,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.417800000041552,
            "unit": "ms",
            "extra": "Min 7.220ms\nMax 7.885ms\nMean 7.540ms\nStdDev 0.277ms\nMedian 7.418ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.8742999999690255,
            "unit": "ms",
            "extra": "Min 7.288ms\nMax 8.061ms\nMean 7.789ms\nStdDev 0.307ms\nMedian 7.874ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.200800000045092,
            "unit": "ms",
            "extra": "Min 17.642ms\nMax 18.850ms\nMean 18.201ms\nStdDev 0.464ms\nMedian 18.201ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009499999919171387,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3027999999858366,
            "unit": "ms",
            "extra": "Min 0.299ms\nMax 0.311ms\nMean 0.303ms\nStdDev 0.004ms\nMedian 0.303ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.875699999952303,
            "unit": "ms",
            "extra": "Min 9.571ms\nMax 10.298ms\nMean 9.948ms\nStdDev 0.288ms\nMedian 9.876ms"
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
          "id": "aa1f1b79dbf31d9576dd9573e8c863dd685ea13b",
          "message": "fix(ndk): correct interpolation of new `project.layout.buildDirectory` property (#1258)",
          "timestamp": "2025-05-28T10:09:46+02:00",
          "tree_id": "4245ae38cbb06385b4a7e795849313dfe997dac8",
          "url": "https://github.com/getsentry/sentry-native/commit/aa1f1b79dbf31d9576dd9573e8c863dd685ea13b"
        },
        "date": 1748420081984,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.941500000039014,
            "unit": "ms",
            "extra": "Min 6.894ms\nMax 8.174ms\nMean 7.208ms\nStdDev 0.546ms\nMedian 6.942ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.478300000002491,
            "unit": "ms",
            "extra": "Min 7.292ms\nMax 8.682ms\nMean 7.679ms\nStdDev 0.576ms\nMedian 7.478ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.868200000042634,
            "unit": "ms",
            "extra": "Min 18.442ms\nMax 19.966ms\nMean 19.003ms\nStdDev 0.569ms\nMedian 18.868ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010900000006586197,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.014ms\nMean 0.011ms\nStdDev 0.002ms\nMedian 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3289999999651627,
            "unit": "ms",
            "extra": "Min 0.317ms\nMax 0.380ms\nMean 0.337ms\nStdDev 0.025ms\nMedian 0.329ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.387799999989511,
            "unit": "ms",
            "extra": "Min 10.009ms\nMax 11.288ms\nMean 10.482ms\nStdDev 0.503ms\nMedian 10.388ms"
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
          "id": "6ca83891fdea3f141df66eb6a2f148ef465463c2",
          "message": "fix: use custom page allocator on ps5 (#1257)",
          "timestamp": "2025-05-28T10:16:54+02:00",
          "tree_id": "839d7d4472a60b05ccb61e0105979c5205dd1bf1",
          "url": "https://github.com/getsentry/sentry-native/commit/6ca83891fdea3f141df66eb6a2f148ef465463c2"
        },
        "date": 1748420474518,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.198899999934838,
            "unit": "ms",
            "extra": "Min 7.040ms\nMax 7.362ms\nMean 7.205ms\nStdDev 0.117ms\nMedian 7.199ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.8519999999571155,
            "unit": "ms",
            "extra": "Min 7.694ms\nMax 7.928ms\nMean 7.845ms\nStdDev 0.091ms\nMedian 7.852ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.344500000011976,
            "unit": "ms",
            "extra": "Min 17.691ms\nMax 19.164ms\nMean 18.492ms\nStdDev 0.581ms\nMedian 18.345ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009999999974752427,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.015ms\nMean 0.011ms\nStdDev 0.003ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3092999999125823,
            "unit": "ms",
            "extra": "Min 0.302ms\nMax 0.337ms\nMean 0.316ms\nStdDev 0.016ms\nMedian 0.309ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.04169999998794,
            "unit": "ms",
            "extra": "Min 9.681ms\nMax 10.937ms\nMean 10.116ms\nStdDev 0.500ms\nMedian 10.042ms"
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
          "id": "a1b3947584935eea5680ba54e8914ec913509566",
          "message": "fix: limit proguard rules in the NDK package to local namespaces (#1250)\n\n* fix: limit proguard rules in the NDK package to local namespaces\n\n* rewörding",
          "timestamp": "2025-06-02T08:08:51+02:00",
          "tree_id": "ad71de44e8f71235e89c6e97cd355467f52ddf91",
          "url": "https://github.com/getsentry/sentry-native/commit/a1b3947584935eea5680ba54e8914ec913509566"
        },
        "date": 1748844825844,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.176600000093458,
            "unit": "ms",
            "extra": "Min 6.999ms\nMax 7.295ms\nMean 7.153ms\nStdDev 0.137ms\nMedian 7.177ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.110500000180764,
            "unit": "ms",
            "extra": "Min 7.100ms\nMax 7.602ms\nMean 7.224ms\nStdDev 0.216ms\nMedian 7.111ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.32349999995131,
            "unit": "ms",
            "extra": "Min 17.499ms\nMax 18.526ms\nMean 18.110ms\nStdDev 0.483ms\nMedian 18.323ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30299999980343273,
            "unit": "ms",
            "extra": "Min 0.297ms\nMax 0.330ms\nMean 0.306ms\nStdDev 0.014ms\nMedian 0.303ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.311100000104489,
            "unit": "ms",
            "extra": "Min 9.003ms\nMax 9.548ms\nMean 9.264ms\nStdDev 0.236ms\nMedian 9.311ms"
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
          "id": "9c0950343b3531c797b4c1c87ae6f5b5d1281ed3",
          "message": "fix: close file and return 0 on success when writing raw envelopes (#1260)",
          "timestamp": "2025-06-02T10:54:33+02:00",
          "tree_id": "2d6515f02317a16a2f5e417fe7850a1042cefa60",
          "url": "https://github.com/getsentry/sentry-native/commit/9c0950343b3531c797b4c1c87ae6f5b5d1281ed3"
        },
        "date": 1748854720822,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.392200000026605,
            "unit": "ms",
            "extra": "Min 7.019ms\nMax 7.860ms\nMean 7.364ms\nStdDev 0.328ms\nMedian 7.392ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.03659999996853,
            "unit": "ms",
            "extra": "Min 7.579ms\nMax 11.063ms\nMean 8.509ms\nStdDev 1.444ms\nMedian 8.037ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.58489999995072,
            "unit": "ms",
            "extra": "Min 17.355ms\nMax 19.813ms\nMean 18.515ms\nStdDev 0.894ms\nMedian 18.585ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009299999987888441,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3198000000566026,
            "unit": "ms",
            "extra": "Min 0.302ms\nMax 0.356ms\nMean 0.327ms\nStdDev 0.021ms\nMedian 0.320ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.601599999996324,
            "unit": "ms",
            "extra": "Min 8.898ms\nMax 10.485ms\nMean 9.534ms\nStdDev 0.652ms\nMedian 9.602ms"
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
          "id": "0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a",
          "message": "ci: pin kcov to v43 (#1265)",
          "timestamp": "2025-06-03T10:19:39+02:00",
          "tree_id": "71da9cd13983c4e189589ad750e36b9fe96bccbc",
          "url": "https://github.com/getsentry/sentry-native/commit/0ad2977fb57ff5a61a3cc4734432b9cdc4aa717a"
        },
        "date": 1748939070312,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.095899999967514,
            "unit": "ms",
            "extra": "Min 6.982ms\nMax 7.175ms\nMean 7.081ms\nStdDev 0.085ms\nMedian 7.096ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 14.52480000000378,
            "unit": "ms",
            "extra": "Min 9.069ms\nMax 19.045ms\nMean 13.819ms\nStdDev 3.700ms\nMedian 14.525ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.1603000000905,
            "unit": "ms",
            "extra": "Min 17.713ms\nMax 18.748ms\nMean 18.257ms\nStdDev 0.400ms\nMedian 18.160ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009800000043469481,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.013ms\nMean 0.011ms\nStdDev 0.002ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3136999999924228,
            "unit": "ms",
            "extra": "Min 0.307ms\nMax 0.342ms\nMean 0.319ms\nStdDev 0.014ms\nMedian 0.314ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.80220000008103,
            "unit": "ms",
            "extra": "Min 9.735ms\nMax 10.597ms\nMean 10.047ms\nStdDev 0.398ms\nMedian 9.802ms"
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
          "id": "1fe91217f1ce839f46ef398bdf1f3bf58d99a0da",
          "message": "ci: use Alpine Linux Docker image (#1261)",
          "timestamp": "2025-06-03T11:10:02+02:00",
          "tree_id": "2a2a6c8a0fff494a2d8eb478d6f7350cc26c2125",
          "url": "https://github.com/getsentry/sentry-native/commit/1fe91217f1ce839f46ef398bdf1f3bf58d99a0da"
        },
        "date": 1748942110992,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.832799999983763,
            "unit": "ms",
            "extra": "Min 6.711ms\nMax 7.134ms\nMean 6.872ms\nStdDev 0.163ms\nMedian 6.833ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.2196000000985805,
            "unit": "ms",
            "extra": "Min 7.097ms\nMax 7.949ms\nMean 7.334ms\nStdDev 0.348ms\nMedian 7.220ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.5394000000324,
            "unit": "ms",
            "extra": "Min 16.924ms\nMax 17.632ms\nMean 17.437ms\nStdDev 0.291ms\nMedian 17.539ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012299999980314169,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3013999998984218,
            "unit": "ms",
            "extra": "Min 0.297ms\nMax 0.346ms\nMean 0.316ms\nStdDev 0.024ms\nMedian 0.301ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.446999999909167,
            "unit": "ms",
            "extra": "Min 9.106ms\nMax 9.626ms\nMean 9.421ms\nStdDev 0.208ms\nMedian 9.447ms"
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
          "id": "fc6a314033b1df0ddb9a9ecb51b3268deb7572cb",
          "message": "fix: respect event data when applying/merging scope data (#1253)",
          "timestamp": "2025-06-04T12:32:03+02:00",
          "tree_id": "04848bc8968f055481f86c213cbd3a2f3e6dafa1",
          "url": "https://github.com/getsentry/sentry-native/commit/fc6a314033b1df0ddb9a9ecb51b3268deb7572cb"
        },
        "date": 1749033439600,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.931599999973059,
            "unit": "ms",
            "extra": "Min 6.779ms\nMax 7.353ms\nMean 6.970ms\nStdDev 0.223ms\nMedian 6.932ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.257500000013351,
            "unit": "ms",
            "extra": "Min 8.153ms\nMax 8.712ms\nMean 8.333ms\nStdDev 0.220ms\nMedian 8.258ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.491500000015094,
            "unit": "ms",
            "extra": "Min 19.303ms\nMax 19.853ms\nMean 19.510ms\nStdDev 0.211ms\nMedian 19.492ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01190000000406144,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.000ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.35189999999829524,
            "unit": "ms",
            "extra": "Min 0.340ms\nMax 0.435ms\nMean 0.365ms\nStdDev 0.040ms\nMedian 0.352ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.70279999999002,
            "unit": "ms",
            "extra": "Min 10.281ms\nMax 12.613ms\nMean 11.027ms\nStdDev 0.922ms\nMedian 10.703ms"
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
          "id": "31a8ee35ab3a77fcd201535ea814a3611ad34102",
          "message": "feat: add PS SDK transport support (#1262)\n\n* feat: allow downstream SDKs to implement custom transport\n\n* add \"pshttp\" support to the list of supported ones",
          "timestamp": "2025-06-04T14:10:02+02:00",
          "tree_id": "397dcdefbc9b7cc6aafa6f596959832e93704850",
          "url": "https://github.com/getsentry/sentry-native/commit/31a8ee35ab3a77fcd201535ea814a3611ad34102"
        },
        "date": 1749039308986,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.754000000090855,
            "unit": "ms",
            "extra": "Min 7.438ms\nMax 8.404ms\nMean 7.839ms\nStdDev 0.353ms\nMedian 7.754ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.32689999995273,
            "unit": "ms",
            "extra": "Min 7.126ms\nMax 7.906ms\nMean 7.426ms\nStdDev 0.299ms\nMedian 7.327ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.160900000021684,
            "unit": "ms",
            "extra": "Min 17.025ms\nMax 17.385ms\nMean 17.204ms\nStdDev 0.148ms\nMedian 17.161ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011799999924733129,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.012ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.33540000003995374,
            "unit": "ms",
            "extra": "Min 0.307ms\nMax 0.357ms\nMean 0.333ms\nStdDev 0.020ms\nMedian 0.335ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.452300000020841,
            "unit": "ms",
            "extra": "Min 9.335ms\nMax 9.864ms\nMean 9.557ms\nStdDev 0.222ms\nMedian 9.452ms"
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
          "id": "54cede931f9a76e291be9083d56702c7d20150cd",
          "message": "feat: Add support for capturing events with local scopes (#1248)\n\n* wip: local scopes\n\n* merge breadcrumbs\n\n* add sentry_scope_set_trace\n\n* add sentry_scope_set_fingerprints()\n\n* check fingerprints value type\n\n* document sentry_scope_set_fingerprints() expected type\n\n* Revert sentry_scope_set_trace/transaction\n\n> Transactions/spans do not make sense in this setup since they aren't\n> cloned and cannot be retrieved to create children.\n\n* sentry_malloc -> SENTRY_MAKE\n\n* fix comparing null timestamps when merging breadcrumbs\n\n* take ownership\n\n* update example\n\n* partial revert of unit test changes in a48fea\n\ndon't assume any specific order for breadcrumbs with missing breadcrumbs\n\n* warn once if any breadcrumbs were missing timestamps\n\n* error handling for sentry_value_append()",
          "timestamp": "2025-06-04T16:29:38+02:00",
          "tree_id": "2c4e06d37f7f702c92c3effcb198b108f06a6b70",
          "url": "https://github.com/getsentry/sentry-native/commit/54cede931f9a76e291be9083d56702c7d20150cd"
        },
        "date": 1749047705110,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.415100000002894,
            "unit": "ms",
            "extra": "Min 7.050ms\nMax 15.761ms\nMean 9.028ms\nStdDev 3.767ms\nMedian 7.415ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.554799999979878,
            "unit": "ms",
            "extra": "Min 7.439ms\nMax 7.960ms\nMean 7.672ms\nStdDev 0.222ms\nMedian 7.555ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.189499999972213,
            "unit": "ms",
            "extra": "Min 16.993ms\nMax 20.216ms\nMean 17.751ms\nStdDev 1.381ms\nMedian 17.189ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009300000101575279,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.2962999999454041,
            "unit": "ms",
            "extra": "Min 0.290ms\nMax 0.322ms\nMean 0.302ms\nStdDev 0.014ms\nMedian 0.296ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.241799999927025,
            "unit": "ms",
            "extra": "Min 9.074ms\nMax 9.353ms\nMean 9.215ms\nStdDev 0.133ms\nMedian 9.242ms"
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
          "id": "c56ebdea24486771e28853b78afcd4f6d8ab4833",
          "message": "feat: `crashpad_wait_for_upload` Windows support (#1255)",
          "timestamp": "2025-06-05T18:46:32+02:00",
          "tree_id": "2b213da0b6f44cb2588ac5cd94f4cf3006cbccf2",
          "url": "https://github.com/getsentry/sentry-native/commit/c56ebdea24486771e28853b78afcd4f6d8ab4833"
        },
        "date": 1749142224780,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.856099999936305,
            "unit": "ms",
            "extra": "Min 6.694ms\nMax 7.239ms\nMean 6.900ms\nStdDev 0.207ms\nMedian 6.856ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.555700000011711,
            "unit": "ms",
            "extra": "Min 7.368ms\nMax 7.582ms\nMean 7.498ms\nStdDev 0.104ms\nMedian 7.556ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 16.919000000029882,
            "unit": "ms",
            "extra": "Min 16.754ms\nMax 19.697ms\nMean 17.731ms\nStdDev 1.288ms\nMedian 16.919ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009499999919171387,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3061999999545151,
            "unit": "ms",
            "extra": "Min 0.292ms\nMax 0.310ms\nMean 0.304ms\nStdDev 0.008ms\nMedian 0.306ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.06960000008894,
            "unit": "ms",
            "extra": "Min 8.920ms\nMax 9.106ms\nMean 9.041ms\nStdDev 0.072ms\nMedian 9.070ms"
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
          "id": "9b29c7996e06c200c23c199d95844be8366b16cc",
          "message": "Merge branch 'release/0.9.0'",
          "timestamp": "2025-06-05T17:31:05Z",
          "tree_id": "522307788bf54fcc5ce2e6fa66c7c423134a840c",
          "url": "https://github.com/getsentry/sentry-native/commit/9b29c7996e06c200c23c199d95844be8366b16cc"
        },
        "date": 1749144894370,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.99090000000524,
            "unit": "ms",
            "extra": "Min 6.802ms\nMax 7.332ms\nMean 7.049ms\nStdDev 0.216ms\nMedian 6.991ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.272099999909187,
            "unit": "ms",
            "extra": "Min 7.126ms\nMax 10.003ms\nMean 8.134ms\nStdDev 1.321ms\nMedian 7.272ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.440499999906933,
            "unit": "ms",
            "extra": "Min 16.782ms\nMax 20.282ms\nMean 17.969ms\nStdDev 1.414ms\nMedian 17.440ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009400000067216752,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31149999995250255,
            "unit": "ms",
            "extra": "Min 0.298ms\nMax 0.427ms\nMean 0.331ms\nStdDev 0.055ms\nMedian 0.311ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 8.918100000073537,
            "unit": "ms",
            "extra": "Min 8.882ms\nMax 9.357ms\nMean 9.041ms\nStdDev 0.204ms\nMedian 8.918ms"
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
          "id": "16fa6892134f162ddbfaaa94b230ca5cd8564e0a",
          "message": "fix: introduce malloc/MAKE rv checks if missing (#1234)\n\n+ ensure that none of the test runs into a segfault by asserting on malloc return paths that propagate",
          "timestamp": "2025-06-12T13:29:34+02:00",
          "tree_id": "f8c1c235ae6bbe2d43730436229d5ee7e5ffc7e2",
          "url": "https://github.com/getsentry/sentry-native/commit/16fa6892134f162ddbfaaa94b230ca5cd8564e0a"
        },
        "date": 1749728066377,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.946599999992031,
            "unit": "ms",
            "extra": "Min 6.807ms\nMax 7.696ms\nMean 7.076ms\nStdDev 0.363ms\nMedian 6.947ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.5190000002294255,
            "unit": "ms",
            "extra": "Min 7.093ms\nMax 7.712ms\nMean 7.431ms\nStdDev 0.241ms\nMedian 7.519ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.076499999802763,
            "unit": "ms",
            "extra": "Min 16.742ms\nMax 17.188ms\nMean 17.030ms\nStdDev 0.180ms\nMedian 17.076ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010199999906035373,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.33119999989139615,
            "unit": "ms",
            "extra": "Min 0.312ms\nMax 0.424ms\nMean 0.346ms\nStdDev 0.045ms\nMedian 0.331ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.36890000002677,
            "unit": "ms",
            "extra": "Min 9.207ms\nMax 9.515ms\nMean 9.347ms\nStdDev 0.116ms\nMedian 9.369ms"
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
          "id": "1bf8db1646a8e6d3b497f3d173368192b75e05ba",
          "message": "ci: drop windows-2019 runner images (#1274)",
          "timestamp": "2025-06-13T11:51:12+02:00",
          "tree_id": "732b83a0139c34a3b7fdfcb4247c8600cd7022b6",
          "url": "https://github.com/getsentry/sentry-native/commit/1bf8db1646a8e6d3b497f3d173368192b75e05ba"
        },
        "date": 1749808568805,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.146900000042478,
            "unit": "ms",
            "extra": "Min 7.824ms\nMax 8.813ms\nMean 8.234ms\nStdDev 0.423ms\nMedian 8.147ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.513799999946059,
            "unit": "ms",
            "extra": "Min 8.259ms\nMax 8.871ms\nMean 8.521ms\nStdDev 0.260ms\nMedian 8.514ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 21.51699999990342,
            "unit": "ms",
            "extra": "Min 20.260ms\nMax 21.897ms\nMean 21.275ms\nStdDev 0.677ms\nMedian 21.517ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.017500000012660166,
            "unit": "ms",
            "extra": "Min 0.016ms\nMax 0.023ms\nMean 0.018ms\nStdDev 0.003ms\nMedian 0.018ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3534999999601496,
            "unit": "ms",
            "extra": "Min 0.335ms\nMax 0.456ms\nMean 0.370ms\nStdDev 0.049ms\nMedian 0.353ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.487099999977545,
            "unit": "ms",
            "extra": "Min 9.980ms\nMax 10.960ms\nMean 10.518ms\nStdDev 0.420ms\nMedian 10.487ms"
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
          "id": "7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8",
          "message": "feat: Support modifying attachments after init (continued) (#1266)\n\n* feat: Support modifying attachments after init\n\nMoves the attachments to the scope, and adds `sentry_add_attachment` and\n`sentry_remove_attachment` and wstr variants that modify this attachment\nlist after calling init. Attachments are identified by their path.\n\n* feat: pass added and removed attachments to the backend\n\n* add `_n`\n\n* scope api\n\n* merge & apply attachments\n\n* update note on attachments\n\n* integration tests\n\n* Update README.md\n\n* Update CHANGELOG.md\n\n* Apply suggestions from code review\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>\n\n* remove ticks\n\n* Apply more suggestions from code review\n\n* De-duplicate envelope attachment code\n\n- remove sentry__apply_attachments_to_envelope\n- add sentry__envelope_add_attachments\n- reuse sentry__envelope_add_attachment\n\n* sentry_add_attachment -> sentry_add_attachment_path\n\n* Update CHANGELOG.md\n\n* fixup: missed rename\n\n* fixup: another missed rename\n\n* remove_attachmentw() without _path\n\n* revise sentry_attach_file & removal\n\n* fix windows\n\n* Update CHANGELOG.md\n\n* clean up\n\n* fix attachments_add_remove on windows\n\n* Update CHANGELOG.md & NOTE on attachments\n\n* Update external/crashpad\n\n---------\n\nCo-authored-by: Arpad Borsos <arpad.borsos@googlemail.com>\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-06-13T14:02:33+02:00",
          "tree_id": "cb94ac9185e31f5e47c7ebc651235c14006bf020",
          "url": "https://github.com/getsentry/sentry-native/commit/7ab3786ad5a60b26d1e0a105faadf6b5c6183cc8"
        },
        "date": 1749816418230,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.112600000027669,
            "unit": "ms",
            "extra": "Min 6.979ms\nMax 7.785ms\nMean 7.262ms\nStdDev 0.331ms\nMedian 7.113ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.269100000030448,
            "unit": "ms",
            "extra": "Min 7.197ms\nMax 7.346ms\nMean 7.271ms\nStdDev 0.063ms\nMedian 7.269ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.21739999993588,
            "unit": "ms",
            "extra": "Min 16.987ms\nMax 17.570ms\nMean 17.239ms\nStdDev 0.244ms\nMedian 17.217ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00959999988481286,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3127999998469022,
            "unit": "ms",
            "extra": "Min 0.305ms\nMax 0.342ms\nMean 0.319ms\nStdDev 0.015ms\nMedian 0.313ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.250600000086706,
            "unit": "ms",
            "extra": "Min 9.028ms\nMax 9.420ms\nMean 9.247ms\nStdDev 0.146ms\nMedian 9.251ms"
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
          "id": "d823acbc116d19393c3552a123ab7e483c383b03",
          "message": "docs: sync return values of AIX dladdr with implementation (#1273)",
          "timestamp": "2025-06-13T15:22:47+02:00",
          "tree_id": "abde47443e4fe5f6a7a7f6e22cff13dfa090ffb9",
          "url": "https://github.com/getsentry/sentry-native/commit/d823acbc116d19393c3552a123ab7e483c383b03"
        },
        "date": 1749821256975,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.403700000168101,
            "unit": "ms",
            "extra": "Min 7.103ms\nMax 7.712ms\nMean 7.424ms\nStdDev 0.267ms\nMedian 7.404ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.9491999999845575,
            "unit": "ms",
            "extra": "Min 7.655ms\nMax 9.573ms\nMean 8.253ms\nStdDev 0.789ms\nMedian 7.949ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.64849999992657,
            "unit": "ms",
            "extra": "Min 18.303ms\nMax 20.856ms\nMean 19.433ms\nStdDev 1.254ms\nMedian 18.648ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009999999974752427,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30629999992015655,
            "unit": "ms",
            "extra": "Min 0.303ms\nMax 0.311ms\nMean 0.307ms\nStdDev 0.003ms\nMedian 0.306ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.488400000009278,
            "unit": "ms",
            "extra": "Min 9.145ms\nMax 10.245ms\nMean 9.582ms\nStdDev 0.404ms\nMedian 9.488ms"
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
          "id": "931c468cb6dcb8856c60cd113ef044d9881ef62e",
          "message": "test: define individual unit tests for CTEST (#1244)\n\n* test: define individual unit tests for CTEST\n\n* chore: add SENTRY_CTEST_INDIVIDUAL option\n\n* rename the aggregate test target to \"unit-tests\"",
          "timestamp": "2025-06-16T12:19:00+02:00",
          "tree_id": "a5600d12fbaa848014d06846b9b5d5ea7bf1a63a",
          "url": "https://github.com/getsentry/sentry-native/commit/931c468cb6dcb8856c60cd113ef044d9881ef62e"
        },
        "date": 1750069414254,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.299400000079004,
            "unit": "ms",
            "extra": "Min 7.003ms\nMax 7.360ms\nMean 7.199ms\nStdDev 0.175ms\nMedian 7.299ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.9860999999255,
            "unit": "ms",
            "extra": "Min 7.630ms\nMax 8.448ms\nMean 8.045ms\nStdDev 0.316ms\nMedian 7.986ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.72809999999936,
            "unit": "ms",
            "extra": "Min 17.495ms\nMax 18.392ms\nMean 17.909ms\nStdDev 0.427ms\nMedian 17.728ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013500000022759195,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.019ms\nMean 0.015ms\nStdDev 0.003ms\nMedian 0.014ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.34740000000965665,
            "unit": "ms",
            "extra": "Min 0.303ms\nMax 0.402ms\nMean 0.346ms\nStdDev 0.041ms\nMedian 0.347ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.960000000091895,
            "unit": "ms",
            "extra": "Min 9.252ms\nMax 10.018ms\nMean 9.830ms\nStdDev 0.324ms\nMedian 9.960ms"
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
          "id": "455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8",
          "message": "feat: add sentry_attachment_set_content_type() (#1276)\n\n* feat: add sentry_attachment_set_content_type()\n\n* drop content_type_owned\n\n* add _n",
          "timestamp": "2025-06-16T15:55:01+02:00",
          "tree_id": "0af59a742a894ffc9893e993308fbf3fa7f147b9",
          "url": "https://github.com/getsentry/sentry-native/commit/455f0ef2be4a6c54c2453c1813d6dbe3d5cd9ed8"
        },
        "date": 1750082424603,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.685300000024654,
            "unit": "ms",
            "extra": "Min 8.040ms\nMax 8.989ms\nMean 8.599ms\nStdDev 0.392ms\nMedian 8.685ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.454699999902004,
            "unit": "ms",
            "extra": "Min 8.026ms\nMax 8.640ms\nMean 8.396ms\nStdDev 0.240ms\nMedian 8.455ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.713599999955477,
            "unit": "ms",
            "extra": "Min 18.357ms\nMax 19.132ms\nMean 18.731ms\nStdDev 0.275ms\nMedian 18.714ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009800000043469481,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3460000000359287,
            "unit": "ms",
            "extra": "Min 0.311ms\nMax 0.387ms\nMean 0.346ms\nStdDev 0.029ms\nMedian 0.346ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.82979999980671,
            "unit": "ms",
            "extra": "Min 9.620ms\nMax 9.932ms\nMean 9.813ms\nStdDev 0.128ms\nMedian 9.830ms"
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
          "id": "99e598206296b63301919b9bc20e36c39adb51a1",
          "message": "chore: `breakpad` upstream update (#1277)\n\n* update breakpad + lss\n\n* CHANGELOG.md\n\n* breakpad post-merge",
          "timestamp": "2025-06-17T12:42:03+02:00",
          "tree_id": "3bc4293c0013fb354981e4896e27aa66ea889b16",
          "url": "https://github.com/getsentry/sentry-native/commit/99e598206296b63301919b9bc20e36c39adb51a1"
        },
        "date": 1750157224423,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.24560000000929,
            "unit": "ms",
            "extra": "Min 7.782ms\nMax 8.419ms\nMean 8.151ms\nStdDev 0.249ms\nMedian 8.246ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.680799999979172,
            "unit": "ms",
            "extra": "Min 8.072ms\nMax 8.769ms\nMean 8.496ms\nStdDev 0.334ms\nMedian 8.681ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.592300000136674,
            "unit": "ms",
            "extra": "Min 18.376ms\nMax 19.933ms\nMean 19.055ms\nStdDev 0.775ms\nMedian 18.592ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011100000165242818,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.012ms\nMean 0.011ms\nStdDev 0.001ms\nMedian 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31160000003183086,
            "unit": "ms",
            "extra": "Min 0.310ms\nMax 0.350ms\nMean 0.319ms\nStdDev 0.017ms\nMedian 0.312ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.770400000206791,
            "unit": "ms",
            "extra": "Min 9.639ms\nMax 10.017ms\nMean 9.812ms\nStdDev 0.154ms\nMedian 9.770ms"
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
          "id": "6aaccb486304e891170fd7489c4d2cf76f8bfc1d",
          "message": "docs: remove `sentry_event_value_add_stacktrace()` from example (#1281)",
          "timestamp": "2025-06-20T15:16:54+02:00",
          "tree_id": "5270710c3fb3f6847aba3bd3bade5a46ca262e5d",
          "url": "https://github.com/getsentry/sentry-native/commit/6aaccb486304e891170fd7489c4d2cf76f8bfc1d"
        },
        "date": 1750425692568,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.094800000004398,
            "unit": "ms",
            "extra": "Min 6.874ms\nMax 7.681ms\nMean 7.164ms\nStdDev 0.318ms\nMedian 7.095ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.683299999825977,
            "unit": "ms",
            "extra": "Min 7.507ms\nMax 9.377ms\nMean 8.007ms\nStdDev 0.774ms\nMedian 7.683ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.212499999914144,
            "unit": "ms",
            "extra": "Min 17.020ms\nMax 18.024ms\nMean 17.383ms\nStdDev 0.416ms\nMedian 17.212ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010400000064691994,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.013ms\nMean 0.011ms\nStdDev 0.002ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3153000000111206,
            "unit": "ms",
            "extra": "Min 0.295ms\nMax 0.358ms\nMean 0.317ms\nStdDev 0.025ms\nMedian 0.315ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.334299999864015,
            "unit": "ms",
            "extra": "Min 9.308ms\nMax 13.369ms\nMean 10.178ms\nStdDev 1.787ms\nMedian 9.334ms"
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
          "id": "889d59aa2d165850e7fcdbedcb9770144638874f",
          "message": "chore: xbox compilation fixes and cleanup (#1284)\n\n* fix: xbox compilation\n\n* replace checks of _GAMING_XBOX_SCARLETT with SENTRY_PLATFORM_XBOX_SCARLETT\n\n* test: skip tests for missing features",
          "timestamp": "2025-06-24T14:52:27+02:00",
          "tree_id": "c6bb64d8f4f63b2374351a0a9ea016cf2782b76e",
          "url": "https://github.com/getsentry/sentry-native/commit/889d59aa2d165850e7fcdbedcb9770144638874f"
        },
        "date": 1750769800189,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.307999999966341,
            "unit": "ms",
            "extra": "Min 7.148ms\nMax 8.087ms\nMean 7.548ms\nStdDev 0.420ms\nMedian 7.308ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.9616000000442,
            "unit": "ms",
            "extra": "Min 7.450ms\nMax 10.111ms\nMean 8.359ms\nStdDev 1.028ms\nMedian 7.962ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 21.24509999998736,
            "unit": "ms",
            "extra": "Min 18.737ms\nMax 29.595ms\nMean 22.270ms\nStdDev 4.424ms\nMedian 21.245ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00959999988481286,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3087000000050466,
            "unit": "ms",
            "extra": "Min 0.299ms\nMax 0.360ms\nMean 0.321ms\nStdDev 0.025ms\nMedian 0.309ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.321600000021135,
            "unit": "ms",
            "extra": "Min 9.135ms\nMax 9.569ms\nMean 9.349ms\nStdDev 0.169ms\nMedian 9.322ms"
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
          "id": "9dd7d6c61a1a3406fc613b4e2bad40baacc5453f",
          "message": "feat: support attaching bytes (#1275)\n\n* feat: Support attaching bytes\n\n* fix: move to_crashpad_attachment out of extern C\n\n> warning C4190: 'to_crashpad_attachment' has C-linkage specified,\n> but returns UDT 'crashpad::Attachment' which is incompatible with C\n\n* fix: lint\n\n* fix: unreachable\n\n* test: integration\n\n* test: rename\n\n* test: attaching bytes to crashpad is supported on win32 and linux\n\n* crashpad: dump byte attachments on disk\n\n* fix: windows\n\n* let crashpad ensure unique file names\n\n* fix sentry__attachment_from_buffer\n\n* clean up unused uuid\n\n* Update external/crashpad\n\n* alternative: ensure unique file in sentry_backend_crashpad\n\n* clean up\n\n* clean up more\n\n* switch to std::filesystem\n\n* fix leaks in backends\n\n* add sentry__attachment_from_path for convenience and to reduce diff\n\n* fix self-review findings\n\n* revert accidental ws changes\n\n* fix attachment_clone\n\ntype & content_type are passed separately and content_type is cloned in\nsentry__attachments_add()\n\n* unit-testable sentry__path_unique() to back the \"-N.tar.gz\" claims\n\n* include <string>\n\n* ref: unique paths for byte attachments\n\n* add note about unique file names with crashpad\n\n* add missing null checks for screenshots\n\n* attachment_clone: add missing error handling\n\n* add note and missing test for buffer attachment comparison\n\n* Bump external/crashpad\n\n* Update external/crashpad\n\n* attachment_eq: clarify with a comment\n\n* document behavior regarding duplicate attachments\n\n* sentry__attachments_remove: replace attachment_eq with ptr cmp",
          "timestamp": "2025-06-24T16:27:14+02:00",
          "tree_id": "9e05ee000fa10a5e67a901a9fb21081b48123122",
          "url": "https://github.com/getsentry/sentry-native/commit/9dd7d6c61a1a3406fc613b4e2bad40baacc5453f"
        },
        "date": 1750775522296,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 6.978399999979956,
            "unit": "ms",
            "extra": "Min 6.948ms\nMax 7.137ms\nMean 7.015ms\nStdDev 0.078ms\nMedian 6.978ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.523500000047534,
            "unit": "ms",
            "extra": "Min 7.373ms\nMax 8.009ms\nMean 7.581ms\nStdDev 0.255ms\nMedian 7.524ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.323499999974956,
            "unit": "ms",
            "extra": "Min 17.193ms\nMax 32.374ms\nMean 20.372ms\nStdDev 6.712ms\nMedian 17.323ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00969999996414117,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.30839999999443535,
            "unit": "ms",
            "extra": "Min 0.299ms\nMax 0.312ms\nMean 0.307ms\nStdDev 0.005ms\nMedian 0.308ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.430100000031416,
            "unit": "ms",
            "extra": "Min 8.961ms\nMax 9.523ms\nMean 9.361ms\nStdDev 0.228ms\nMedian 9.430ms"
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
          "id": "75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79",
          "message": "fix: xbox config typo (#1286)",
          "timestamp": "2025-06-24T20:48:09+02:00",
          "tree_id": "1b79f2f0327ba5e69ff5d101608f6752ebebb5b5",
          "url": "https://github.com/getsentry/sentry-native/commit/75e5f8cad312232a0fa5b12bc96bcf6eb5e2fb79"
        },
        "date": 1750791228271,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.882200000040029,
            "unit": "ms",
            "extra": "Min 6.951ms\nMax 8.312ms\nMean 7.644ms\nStdDev 0.582ms\nMedian 7.882ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.33650000004127,
            "unit": "ms",
            "extra": "Min 8.255ms\nMax 8.527ms\nMean 8.359ms\nStdDev 0.105ms\nMedian 8.337ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 18.301800000017465,
            "unit": "ms",
            "extra": "Min 17.613ms\nMax 18.856ms\nMean 18.302ms\nStdDev 0.536ms\nMedian 18.302ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00959999988481286,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31349999994745303,
            "unit": "ms",
            "extra": "Min 0.301ms\nMax 0.330ms\nMean 0.315ms\nStdDev 0.010ms\nMedian 0.313ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.28930000006767,
            "unit": "ms",
            "extra": "Min 9.842ms\nMax 10.835ms\nMean 10.306ms\nStdDev 0.385ms\nMedian 10.289ms"
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
          "id": "7646cac9c760c6734741d96320ec1e084b1e00f3",
          "message": "feat: add `sentry_attachment_set_filename()` (#1285)\n\n* feat: add `sentry_attachment_set_filename()`\n\n* add missing null check\n\n* crashpad: adapt ensure_unique_path",
          "timestamp": "2025-06-25T09:10:39+02:00",
          "tree_id": "6662413ce89811f88fc9b2b01f77313e006a6d87",
          "url": "https://github.com/getsentry/sentry-native/commit/7646cac9c760c6734741d96320ec1e084b1e00f3"
        },
        "date": 1750835697841,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.124499999918044,
            "unit": "ms",
            "extra": "Min 7.043ms\nMax 7.277ms\nMean 7.150ms\nStdDev 0.103ms\nMedian 7.124ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.851800000025833,
            "unit": "ms",
            "extra": "Min 7.305ms\nMax 8.191ms\nMean 7.743ms\nStdDev 0.405ms\nMedian 7.852ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.312600000195744,
            "unit": "ms",
            "extra": "Min 17.200ms\nMax 17.369ms\nMean 17.287ms\nStdDev 0.074ms\nMedian 17.313ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009300000101575279,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31199999989439675,
            "unit": "ms",
            "extra": "Min 0.301ms\nMax 0.357ms\nMean 0.321ms\nStdDev 0.022ms\nMedian 0.312ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.40719999994144,
            "unit": "ms",
            "extra": "Min 9.356ms\nMax 9.728ms\nMean 9.493ms\nStdDev 0.160ms\nMedian 9.407ms"
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
          "id": "28fb3edd0ef3638059ab86d4f734f6cc5f0c9652",
          "message": "meta: identify Xbox as a separate SDK name (#1287)\n\n* Update CHANGELOG.md\n* move static sdk identification + versioning below platform defs in order to reuse the platform defs rather than external ones.",
          "timestamp": "2025-06-25T10:52:07+02:00",
          "tree_id": "30ce0a45de61b725825d2c3bbd76a94c01d5a8bd",
          "url": "https://github.com/getsentry/sentry-native/commit/28fb3edd0ef3638059ab86d4f734f6cc5f0c9652"
        },
        "date": 1750841862828,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.010600000057821,
            "unit": "ms",
            "extra": "Min 6.875ms\nMax 7.213ms\nMean 7.020ms\nStdDev 0.130ms\nMedian 7.011ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.370599999831029,
            "unit": "ms",
            "extra": "Min 7.262ms\nMax 7.650ms\nMean 7.424ms\nStdDev 0.145ms\nMedian 7.371ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.109800000071118,
            "unit": "ms",
            "extra": "Min 16.964ms\nMax 17.274ms\nMean 17.104ms\nStdDev 0.117ms\nMedian 17.110ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011000000085914508,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.013ms\nMean 0.011ms\nStdDev 0.001ms\nMedian 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3154999999424035,
            "unit": "ms",
            "extra": "Min 0.309ms\nMax 0.320ms\nMean 0.315ms\nStdDev 0.004ms\nMedian 0.315ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.180400000104783,
            "unit": "ms",
            "extra": "Min 9.046ms\nMax 9.395ms\nMean 9.201ms\nStdDev 0.151ms\nMedian 9.180ms"
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
          "id": "e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112",
          "message": "chore: enable PS stack unwinding and module resolution (#1282)\n\n* chore: enable PS stack unwinding and module resolution\n\n* symbolize stacktrace automatically on PS",
          "timestamp": "2025-06-25T11:09:57+02:00",
          "tree_id": "f660999902c870f1ee163adc984d2245c84d258a",
          "url": "https://github.com/getsentry/sentry-native/commit/e2ea52c0d2d858fcb1dbfd8f835ea3dfabab7112"
        },
        "date": 1750842874028,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.0473000005222275,
            "unit": "ms",
            "extra": "Min 6.887ms\nMax 7.738ms\nMean 7.173ms\nStdDev 0.334ms\nMedian 7.047ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.728700000370736,
            "unit": "ms",
            "extra": "Min 7.303ms\nMax 8.074ms\nMean 7.695ms\nStdDev 0.316ms\nMedian 7.729ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.13519999975688,
            "unit": "ms",
            "extra": "Min 16.967ms\nMax 17.232ms\nMean 17.128ms\nStdDev 0.101ms\nMedian 17.135ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000487605575,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.010ms\nMean 0.009ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31989999934012303,
            "unit": "ms",
            "extra": "Min 0.297ms\nMax 0.348ms\nMean 0.321ms\nStdDev 0.020ms\nMedian 0.320ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.50120000015886,
            "unit": "ms",
            "extra": "Min 9.142ms\nMax 9.771ms\nMean 9.470ms\nStdDev 0.247ms\nMedian 9.501ms"
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
          "id": "3f7bbd6193bda6814a7258da31971f21bf61da26",
          "message": "fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined (#1279)\n\n* fix: compilation error if SENTRY_TEST_PATH_PREFIX is defined\n\n* run unit tests with custom path prefix\n\n* linter issue\n\n* fix cmake.py\n\n* Update tests/unit/sentry_testsupport.h",
          "timestamp": "2025-06-25T11:10:59+02:00",
          "tree_id": "be3a9dccafda19c04122b5283e8e33c4e4f818a2",
          "url": "https://github.com/getsentry/sentry-native/commit/3f7bbd6193bda6814a7258da31971f21bf61da26"
        },
        "date": 1750842906279,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.718999999951848,
            "unit": "ms",
            "extra": "Min 7.176ms\nMax 8.211ms\nMean 7.700ms\nStdDev 0.465ms\nMedian 7.719ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.564699999875302,
            "unit": "ms",
            "extra": "Min 7.380ms\nMax 8.527ms\nMean 7.724ms\nStdDev 0.464ms\nMedian 7.565ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 24.53260000015689,
            "unit": "ms",
            "extra": "Min 22.515ms\nMax 47.073ms\nMean 28.691ms\nStdDev 10.420ms\nMedian 24.533ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013099999932819628,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.044ms\nMean 0.019ms\nStdDev 0.014ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.31979999994291575,
            "unit": "ms",
            "extra": "Min 0.305ms\nMax 0.329ms\nMean 0.318ms\nStdDev 0.010ms\nMedian 0.320ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.86020000004828,
            "unit": "ms",
            "extra": "Min 9.394ms\nMax 11.146ms\nMean 10.061ms\nStdDev 0.705ms\nMedian 9.860ms"
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
          "id": "6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc",
          "message": "Merge branch 'release/0.9.1'",
          "timestamp": "2025-06-25T11:07:11Z",
          "tree_id": "51b4af13587545183a4c80ccce4bad6490ba3728",
          "url": "https://github.com/getsentry/sentry-native/commit/6d3c836d8f4eda454fd7f8a75bbd1dd88577dbdc"
        },
        "date": 1750849877101,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 7.6525000001765875,
            "unit": "ms",
            "extra": "Min 7.385ms\nMax 7.954ms\nMean 7.652ms\nStdDev 0.222ms\nMedian 7.653ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 7.444500000019616,
            "unit": "ms",
            "extra": "Min 7.163ms\nMax 8.046ms\nMean 7.501ms\nStdDev 0.351ms\nMedian 7.445ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 17.983799999910843,
            "unit": "ms",
            "extra": "Min 17.383ms\nMax 23.735ms\nMean 19.726ms\nStdDev 2.992ms\nMedian 17.984ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.013ms\nMean 0.010ms\nStdDev 0.002ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3074999999626016,
            "unit": "ms",
            "extra": "Min 0.292ms\nMax 0.318ms\nMean 0.305ms\nStdDev 0.013ms\nMedian 0.307ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 9.34729999994488,
            "unit": "ms",
            "extra": "Min 8.734ms\nMax 9.472ms\nMean 9.210ms\nStdDev 0.319ms\nMedian 9.347ms"
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
          "id": "d3eef89221f127831ea58ac06797c00c96a1d63c",
          "message": "chore: add clangd .cache to .gitignore (#1291)",
          "timestamp": "2025-06-30T14:43:53+02:00",
          "tree_id": "92e0c2dbe387b8c88d769a05c4b5e6c6f248dff7",
          "url": "https://github.com/getsentry/sentry-native/commit/d3eef89221f127831ea58ac06797c00c96a1d63c"
        },
        "date": 1751287684129,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.361500000034994,
            "unit": "ms",
            "extra": "Min 7.840ms\nMax 10.071ms\nMean 8.767ms\nStdDev 1.009ms\nMedian 8.362ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.264500000028875,
            "unit": "ms",
            "extra": "Min 8.429ms\nMax 33.637ms\nMean 14.111ms\nStdDev 10.951ms\nMedian 9.265ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.052300000031664,
            "unit": "ms",
            "extra": "Min 19.739ms\nMax 27.874ms\nMean 22.217ms\nStdDev 3.498ms\nMedian 20.052ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01150000002780871,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.012ms\nMean 0.011ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3764000000501255,
            "unit": "ms",
            "extra": "Min 0.302ms\nMax 0.431ms\nMean 0.371ms\nStdDev 0.046ms\nMedian 0.376ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.750700000016877,
            "unit": "ms",
            "extra": "Min 9.911ms\nMax 13.002ms\nMean 11.272ms\nStdDev 1.436ms\nMedian 10.751ms"
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
          "id": "6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4",
          "message": "ci: update kcov (#1292)\n\n* pin kcov at 8afe9f29c58ef575877664c7ba209328233b70cc",
          "timestamp": "2025-06-30T16:23:35+02:00",
          "tree_id": "fe98eff320080abd8001df117dedf875aac1d839",
          "url": "https://github.com/getsentry/sentry-native/commit/6a92fe57a2420b64c4137d5c85d4bb6a03d8e1d4"
        },
        "date": 1751293677064,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.062599999992926,
            "unit": "ms",
            "extra": "Min 8.888ms\nMax 9.258ms\nMean 9.063ms\nStdDev 0.132ms\nMedian 9.063ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.388599999965663,
            "unit": "ms",
            "extra": "Min 9.254ms\nMax 10.030ms\nMean 9.572ms\nStdDev 0.358ms\nMedian 9.389ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 21.038599999997132,
            "unit": "ms",
            "extra": "Min 20.447ms\nMax 22.313ms\nMean 21.136ms\nStdDev 0.728ms\nMedian 21.039ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011800000038419967,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3895000000397886,
            "unit": "ms",
            "extra": "Min 0.324ms\nMax 0.392ms\nMean 0.368ms\nStdDev 0.032ms\nMedian 0.390ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.26899999997022,
            "unit": "ms",
            "extra": "Min 10.815ms\nMax 11.399ms\nMean 11.164ms\nStdDev 0.243ms\nMedian 11.269ms"
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
          "id": "b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a",
          "message": "feat: add `sentry_clear_attachments()` (#1290)\n\n* feat: add `sentry_clear_attachments()`\n\n* crashpad: clear attachments in one go\n\n* Bump external/crashpad\n\n* Update CHANGELOG.md\n\n* revise remove_buffer_file()\n\n* add integration test\n\n* revert clear_attachments_func\n\nan integration test revealed that it was too effective clearing even\nbreadcrumb attachments\n\n* swap lock order",
          "timestamp": "2025-06-30T20:49:54+02:00",
          "tree_id": "6e201fe7cb9ba9838bcaf761a2f37e56a20695de",
          "url": "https://github.com/getsentry/sentry-native/commit/b24a2ef07bcb5016f2bf0d86bd5734ee53d8b38a"
        },
        "date": 1751309649994,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.317900000008649,
            "unit": "ms",
            "extra": "Min 8.186ms\nMax 8.583ms\nMean 8.362ms\nStdDev 0.186ms\nMedian 8.318ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.924399999988509,
            "unit": "ms",
            "extra": "Min 9.242ms\nMax 10.408ms\nMean 9.921ms\nStdDev 0.504ms\nMedian 9.924ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.99759999999651,
            "unit": "ms",
            "extra": "Min 21.229ms\nMax 27.266ms\nMean 23.978ms\nStdDev 2.778ms\nMedian 22.998ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009800000043469481,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.010ms\nMean 0.010ms\nStdDev 0.000ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3404999999929714,
            "unit": "ms",
            "extra": "Min 0.323ms\nMax 0.400ms\nMean 0.347ms\nStdDev 0.031ms\nMedian 0.340ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.693299999957162,
            "unit": "ms",
            "extra": "Min 10.528ms\nMax 10.918ms\nMean 10.696ms\nStdDev 0.147ms\nMedian 10.693ms"
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
          "id": "3d9c118b7314f76c2350dfdd819e39feda033bf3",
          "message": "feat: add `sentry_regenerate_trace()` (#1293)\n\n* add generate_trace() + test\n\n* changelog\n\n* fix\n\n* fix memleak\n\n* remove const to allow sentry_free\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-01T11:14:09+02:00",
          "tree_id": "abfab1993ac4ca8f50dc33019aa6de9b251dd9ae",
          "url": "https://github.com/getsentry/sentry-native/commit/3d9c118b7314f76c2350dfdd819e39feda033bf3"
        },
        "date": 1751361532917,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.293399999990015,
            "unit": "ms",
            "extra": "Min 8.756ms\nMax 9.356ms\nMean 9.165ms\nStdDev 0.247ms\nMedian 9.293ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.262799999987692,
            "unit": "ms",
            "extra": "Min 9.059ms\nMax 10.380ms\nMean 9.433ms\nStdDev 0.545ms\nMedian 9.263ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.073999999975058,
            "unit": "ms",
            "extra": "Min 20.631ms\nMax 23.394ms\nMean 21.898ms\nStdDev 1.141ms\nMedian 22.074ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.015300000086426735,
            "unit": "ms",
            "extra": "Min 0.014ms\nMax 0.025ms\nMean 0.017ms\nStdDev 0.004ms\nMedian 0.015ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.33849999999802094,
            "unit": "ms",
            "extra": "Min 0.334ms\nMax 0.525ms\nMean 0.388ms\nStdDev 0.083ms\nMedian 0.338ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.830300000066018,
            "unit": "ms",
            "extra": "Min 10.162ms\nMax 12.657ms\nMean 11.097ms\nStdDev 0.963ms\nMedian 10.830ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "bruno@brunogarcia.com",
            "name": "Bruno Garcia",
            "username": "bruno-garcia"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e0b3f8bba161a732463de2f32e00e89d154f7363",
          "message": "compiles on xbox one (#1294)\n\n* feat: xbox one support\n\n* define\n\n* wip\n\n* multiline command\n\n* it builds\n\n* gxdk_base_toolchain.cmake\n\n* update external/breakpad\n\n* fix'n'tweak xbox defines\n\n* Revert \"gxdk_base_toolchain.cmake\"\n\nThis reverts commit 18541aa8e3cada297d5212932dc1ec898ed5d5e6.\n\n* gxdk_toolchain.cmake - take 2\n\nhttps://github.com/microsoft/Xbox-GDK-Samples/blob/aa45b831e7a71160a69a7d13e9d74844dc6aa210/Samples/Tools/CMakeGDKExample/gxdk_toolchain.cmake\n\n* fix formatting\n\n* toolchain md\n\n* changelog\n\n* Update external/breakpad\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-03T16:28:39-04:00",
          "tree_id": "53f3f9cfc8842ce0d121144d90fddafb2b803f5c",
          "url": "https://github.com/getsentry/sentry-native/commit/e0b3f8bba161a732463de2f32e00e89d154f7363"
        },
        "date": 1751574751318,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.255099999985305,
            "unit": "ms",
            "extra": "Min 7.875ms\nMax 8.857ms\nMean 8.333ms\nStdDev 0.384ms\nMedian 8.255ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.216000000004442,
            "unit": "ms",
            "extra": "Min 8.172ms\nMax 17.170ms\nMean 10.066ms\nStdDev 3.975ms\nMedian 8.216ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.60529999996652,
            "unit": "ms",
            "extra": "Min 19.208ms\nMax 20.925ms\nMean 19.857ms\nStdDev 0.674ms\nMedian 19.605ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009099999999762076,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.009ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3143999999792868,
            "unit": "ms",
            "extra": "Min 0.295ms\nMax 0.357ms\nMean 0.321ms\nStdDev 0.025ms\nMedian 0.314ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.226699999975608,
            "unit": "ms",
            "extra": "Min 9.974ms\nMax 10.454ms\nMean 10.190ms\nStdDev 0.210ms\nMedian 10.227ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "rom4ek93@gmail.com",
            "name": "Roman Zavarnitsyn",
            "username": "romtsn"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "98b1bd883824ad6a63c2e6dcc2f07be407c73215",
          "message": "ci(publishing): Use shim ossrh-staging-api server to publish to the new central repo  (#1299)\n\n* Update settings.xml\n\n* Update .craft.yml",
          "timestamp": "2025-07-07T10:21:01+02:00",
          "tree_id": "f6021f5cba3e334cd66083b22d42914810c758ac",
          "url": "https://github.com/getsentry/sentry-native/commit/98b1bd883824ad6a63c2e6dcc2f07be407c73215"
        },
        "date": 1751876745444,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.955700000001343,
            "unit": "ms",
            "extra": "Min 8.733ms\nMax 10.864ms\nMean 9.315ms\nStdDev 0.876ms\nMedian 8.956ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 10.067100000014761,
            "unit": "ms",
            "extra": "Min 9.833ms\nMax 10.588ms\nMean 10.100ms\nStdDev 0.294ms\nMedian 10.067ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 48.16500000003998,
            "unit": "ms",
            "extra": "Min 38.989ms\nMax 49.690ms\nMean 44.906ms\nStdDev 5.335ms\nMedian 48.165ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.021600000081889448,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.023ms\nMean 0.019ms\nStdDev 0.005ms\nMedian 0.022ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3948999999465741,
            "unit": "ms",
            "extra": "Min 0.315ms\nMax 0.463ms\nMean 0.384ms\nStdDev 0.058ms\nMedian 0.395ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 12.232299999936913,
            "unit": "ms",
            "extra": "Min 11.783ms\nMax 25.023ms\nMean 16.900ms\nStdDev 6.812ms\nMedian 12.232ms"
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
          "id": "092202901f9b6fc16e94b7ce4ba2ac2b0afaee41",
          "message": "chore: update xbox toolchain and disable FS-dependent tests (#1302)\n\n* disable tests that require FS access\n\n* disable modulefinder test\n\n* chore: update xbox toolchains\n\n* fixup\n\n* try to reenable modulefinder\n\n* chore: changelog",
          "timestamp": "2025-07-08T18:10:43+02:00",
          "tree_id": "d5a97fdb483155e76f97d6a071eb0df625af382b",
          "url": "https://github.com/getsentry/sentry-native/commit/092202901f9b6fc16e94b7ce4ba2ac2b0afaee41"
        },
        "date": 1751991289400,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.538500000014437,
            "unit": "ms",
            "extra": "Min 8.132ms\nMax 9.230ms\nMean 8.574ms\nStdDev 0.411ms\nMedian 8.539ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.50270000000819,
            "unit": "ms",
            "extra": "Min 8.313ms\nMax 9.006ms\nMean 8.564ms\nStdDev 0.274ms\nMedian 8.503ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.43020000002116,
            "unit": "ms",
            "extra": "Min 19.313ms\nMax 20.815ms\nMean 20.197ms\nStdDev 0.591ms\nMedian 20.430ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.009500000032858225,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3987000000051921,
            "unit": "ms",
            "extra": "Min 0.378ms\nMax 0.415ms\nMean 0.397ms\nStdDev 0.018ms\nMedian 0.399ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.048700000003464,
            "unit": "ms",
            "extra": "Min 10.387ms\nMax 16.554ms\nMean 13.020ms\nStdDev 3.167ms\nMedian 11.049ms"
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
          "id": "9ed9a70d71d98a1f38f8194334c9d1a40bcaed85",
          "message": "meta: add SENTRY_DEPRECATED(msg) macro (#1308)\n\n* meta: add SENTRY_DEPRECATED(msg) macro\n\n* fix deprecation warnings in tests\n\n* Update CHANGELOG.md\n\n* add version checks to be safe\n\n* try to make msvc happy\n\n* add SENTRY_TEST_DEPRECATED\n\n* Revert \"fix deprecation warnings in tests\"\n\nThis reverts commit 0245434bd1a37e3057111e6bb0272c23c2157963.\n\n* Use SENTRY_TEST_DEPRECATED\n\n* fix SENTRY_TEST_DEPRECATED formatting",
          "timestamp": "2025-07-10T18:15:12+02:00",
          "tree_id": "411d1040913b5a1fd7d66d55cd2fd3756c37ce12",
          "url": "https://github.com/getsentry/sentry-native/commit/9ed9a70d71d98a1f38f8194334c9d1a40bcaed85"
        },
        "date": 1752164367056,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.619700000053854,
            "unit": "ms",
            "extra": "Min 7.757ms\nMax 9.398ms\nMean 8.580ms\nStdDev 0.584ms\nMedian 8.620ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.042700000009063,
            "unit": "ms",
            "extra": "Min 8.370ms\nMax 11.002ms\nMean 9.457ms\nStdDev 1.120ms\nMedian 9.043ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.40830000000915,
            "unit": "ms",
            "extra": "Min 19.015ms\nMax 51.271ms\nMean 26.039ms\nStdDev 14.138ms\nMedian 19.408ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012900000001536682,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.014ms\nMean 0.012ms\nStdDev 0.002ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3144999999449283,
            "unit": "ms",
            "extra": "Min 0.311ms\nMax 0.387ms\nMean 0.328ms\nStdDev 0.033ms\nMedian 0.314ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.112300000059804,
            "unit": "ms",
            "extra": "Min 9.918ms\nMax 10.586ms\nMean 10.156ms\nStdDev 0.255ms\nMedian 10.112ms"
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
          "id": "bffc57deb53808b26e11ef2683c05340195f8b0d",
          "message": "feat: implement the new User Feedback API (#1304)\n\n* feat: implement the new User Feedback API\n\nhttps://develop.sentry.dev/sdk/data-model/envelope-items/#user-feedback\n\n* Fix heap-use-after-free\n\n* Update CHANGELOG.md\n\n* remove unused line3_end\n\n* what if we reused the old API?\n\n* use internal id to fix event association in Sentry Web UI\n\n* restore integration test for legacy user report\n\n* convert a deprecated \"user report\" to a new \"user feedback\"\n\n* conversion: note in docs & log info message\n\n* let sentry__envelope_add_user_feedback take ownership\n\n* revert back to option 1\n\n* deprecate sentry_value_new_user_feedback() & sentry_capture_user_feedback()\n\n* fix remaining deprecation warnings\n\n* msvc",
          "timestamp": "2025-07-11T12:28:24+02:00",
          "tree_id": "09bfefa430d3e1795ffea5c12ce54d3a1bc21273",
          "url": "https://github.com/getsentry/sentry-native/commit/bffc57deb53808b26e11ef2683c05340195f8b0d"
        },
        "date": 1752229952316,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 10.583099999962542,
            "unit": "ms",
            "extra": "Min 9.474ms\nMax 14.334ms\nMean 11.311ms\nStdDev 1.983ms\nMedian 10.583ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 12.289499999951659,
            "unit": "ms",
            "extra": "Min 10.487ms\nMax 36.225ms\nMean 16.568ms\nStdDev 11.032ms\nMedian 12.289ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.93340000005628,
            "unit": "ms",
            "extra": "Min 22.382ms\nMax 24.275ms\nMean 23.115ms\nStdDev 0.721ms\nMedian 22.933ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011599999993450183,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.014ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.35260000004200265,
            "unit": "ms",
            "extra": "Min 0.338ms\nMax 0.427ms\nMean 0.369ms\nStdDev 0.036ms\nMedian 0.353ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 12.081099999932121,
            "unit": "ms",
            "extra": "Min 11.743ms\nMax 12.682ms\nMean 12.179ms\nStdDev 0.389ms\nMedian 12.081ms"
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
          "id": "fef7868244cd34c1a6d8650d3c594b0bf2a47314",
          "message": "test: migrate from deprecated `sentry_new_function_transport` (#1310)",
          "timestamp": "2025-07-11T15:15:44+02:00",
          "tree_id": "4ac1f0a0ba99d764784cf93c3294007a95774d7f",
          "url": "https://github.com/getsentry/sentry-native/commit/fef7868244cd34c1a6d8650d3c594b0bf2a47314"
        },
        "date": 1752239986219,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.327799999989338,
            "unit": "ms",
            "extra": "Min 8.157ms\nMax 8.429ms\nMean 8.301ms\nStdDev 0.131ms\nMedian 8.328ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.762799999999515,
            "unit": "ms",
            "extra": "Min 8.550ms\nMax 9.305ms\nMean 8.909ms\nStdDev 0.354ms\nMedian 8.763ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 26.97870000002922,
            "unit": "ms",
            "extra": "Min 20.820ms\nMax 34.396ms\nMean 26.940ms\nStdDev 5.164ms\nMedian 26.979ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01280000003589521,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.016ms\nMean 0.013ms\nStdDev 0.003ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3573999999844091,
            "unit": "ms",
            "extra": "Min 0.338ms\nMax 0.427ms\nMean 0.373ms\nStdDev 0.040ms\nMedian 0.357ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.527200000012726,
            "unit": "ms",
            "extra": "Min 10.291ms\nMax 11.207ms\nMean 10.717ms\nStdDev 0.418ms\nMedian 10.527ms"
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
          "id": "cbaaf1e9590781327ff9eda35b01fefcddd669c0",
          "message": "feat: add dynamic sampling context (#1254)\n\n* initial DSC implementation\n\n* add `sample_rand`\n\n* fix `sample_rand` generation\n\n* fix the fix + add test\n\n* update org_id expected size\n\n* add test + changelog + sample_rand for traces_sampler\n\n* add test + changelog + sample_rand for traces_sampler + dsc for events\n\n* add org_id tests\n\n* fix CHANGELOG.md\n\n* cleanup\n\n* post-merge fix\n\n* post-merge fix v2\n\n* split up test\n\n* fix CHANGELOG.md\n\n* org_id handling update\n\n* asan overflow fix\n\n* add warnings for missing DSC fields\n\n* swap `sentry_set_trace` for `sentry_regenerate_trace`\n\n* add clarifying note\n\n---------\n\nCo-authored-by: Mischan Toosarani-Hausberger <mischan@abovevacant.com>",
          "timestamp": "2025-07-15T11:28:05+02:00",
          "tree_id": "5c65231fce6629c7190a21ad5faca8a1575b5042",
          "url": "https://github.com/getsentry/sentry-native/commit/cbaaf1e9590781327ff9eda35b01fefcddd669c0"
        },
        "date": 1752571928635,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.169100000008257,
            "unit": "ms",
            "extra": "Min 8.747ms\nMax 12.199ms\nMean 9.859ms\nStdDev 1.441ms\nMedian 9.169ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.73319999997102,
            "unit": "ms",
            "extra": "Min 9.354ms\nMax 12.760ms\nMean 10.265ms\nStdDev 1.411ms\nMedian 9.733ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 25.40470000002415,
            "unit": "ms",
            "extra": "Min 21.810ms\nMax 30.882ms\nMean 25.313ms\nStdDev 3.647ms\nMedian 25.405ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.010499999973490048,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.011ms\nMean 0.011ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3687999999897329,
            "unit": "ms",
            "extra": "Min 0.324ms\nMax 0.437ms\nMean 0.374ms\nStdDev 0.047ms\nMedian 0.369ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 12.074200000029123,
            "unit": "ms",
            "extra": "Min 10.762ms\nMax 13.266ms\nMean 12.031ms\nStdDev 1.110ms\nMedian 12.074ms"
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
          "id": "68b918dfd331f218904739b90fe68dd54a1d93ba",
          "message": "test: fix SENTRY_TEST(path_directory) to clean up after itself (#1314)",
          "timestamp": "2025-07-15T13:02:21+02:00",
          "tree_id": "911f165201c3be7020da3d8635688367e6290b8e",
          "url": "https://github.com/getsentry/sentry-native/commit/68b918dfd331f218904739b90fe68dd54a1d93ba"
        },
        "date": 1752577577252,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.192199999915829,
            "unit": "ms",
            "extra": "Min 9.047ms\nMax 9.520ms\nMean 9.286ms\nStdDev 0.215ms\nMedian 9.192ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.569000000055894,
            "unit": "ms",
            "extra": "Min 8.682ms\nMax 11.503ms\nMean 9.982ms\nStdDev 1.365ms\nMedian 9.569ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 21.650200000067343,
            "unit": "ms",
            "extra": "Min 20.141ms\nMax 24.393ms\nMean 21.840ms\nStdDev 1.637ms\nMedian 21.650ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01640000004954345,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.018ms\nMean 0.015ms\nStdDev 0.003ms\nMedian 0.016ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.39260000016838603,
            "unit": "ms",
            "extra": "Min 0.371ms\nMax 0.405ms\nMean 0.390ms\nStdDev 0.015ms\nMedian 0.393ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.517700000013065,
            "unit": "ms",
            "extra": "Min 11.169ms\nMax 12.523ms\nMean 11.619ms\nStdDev 0.540ms\nMedian 11.518ms"
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
          "id": "2b92ca838366ddc76a464e57795c7aaea2265d34",
          "message": "fix(xbox): propagate GDK version to MSBuild (#1312)\n\n* fix: propagate GDK version to MSBuild\n\n* Update CHANGELOG.md",
          "timestamp": "2025-07-15T16:38:52+02:00",
          "tree_id": "f20bb7a137cc3952ac937135be5bd26084682f7f",
          "url": "https://github.com/getsentry/sentry-native/commit/2b92ca838366ddc76a464e57795c7aaea2265d34"
        },
        "date": 1752590571211,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.246399999938149,
            "unit": "ms",
            "extra": "Min 8.658ms\nMax 27.026ms\nMean 13.382ms\nStdDev 7.845ms\nMedian 9.246ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 67.13990000002923,
            "unit": "ms",
            "extra": "Min 20.241ms\nMax 223.731ms\nMean 89.115ms\nStdDev 78.701ms\nMedian 67.140ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.19929999989745,
            "unit": "ms",
            "extra": "Min 22.051ms\nMax 22.580ms\nMean 22.238ms\nStdDev 0.211ms\nMedian 22.199ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012200000014672696,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.014ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.36240000008547213,
            "unit": "ms",
            "extra": "Min 0.338ms\nMax 0.427ms\nMean 0.379ms\nStdDev 0.041ms\nMedian 0.362ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.579699999970217,
            "unit": "ms",
            "extra": "Min 11.349ms\nMax 17.297ms\nMean 12.752ms\nStdDev 2.559ms\nMedian 11.580ms"
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
          "id": "70f38a25655323f2e1e336a8771725c335c41319",
          "message": "ci: enable tsan + fix: rework breadcrumb ringbuffer (#1309)\n\n* adapt python test runner\n\n* fix lock-order inversion in `sentry_start_session()`\n\nThis will typically not hit because the inversion is relevant with the lock acquisition in `sentry_init()` and these two are typically not called from two different threads at the same time.\n\nHowever, consistent lock-order is such a basic sanity step and will guard us from future functions that also require scope- and options-locks.\n\n* properly synchronize `executed_after_shutdown` in `SENTRY_TEST(task_queue)`\n\n* move pre-init in sentry_init outside the options lock\n\n* move set_context outside the scope lock in set_trace so we can prevent unnecessary scope lock recursion\n\n* fix early exit in sentry__scope_get_span_or_transaction (only relevant for tests)\n\n* fix concurrent access to TEST_CHECK macro, which updates an unsynced global in the test-framework\n\n* minor format\n\n* extract the ringbuffer into a separate module\n\nthis allows us to keep state close and remove the need to retrieve max_breadcrumbs from the options after sentry_init(). Which in turn removes the need for nested locks.\n\n* add \"lock handling\" to the contributor docs\n\n* synchronize access to acutest state in the concurrency unit tests\n\n* limit SENTRY_WITH_SCOPE in attachment tests to the lines that actually need the scope\n\n* introduce tsan configs into the matrix\n\n* remove ringbuffer get_len definition\n\n* add tsan.supp + support in test runner\n\n* disable tsan in integration tests that involve DEADLYSIGNALS\n\n* add GCC based TSAN tests as a toolchain cross-check\n\n* clean up\n\n* revert Werror for GCC due to crashpad (not solving this here)\n\n* increase TSAN verbosity for the internal check that fails\n\n* disable all crashpad runs for tsan since it fails an internal check when forking the handler\n\n* format\n\n* re-enable all crashing tests in the http/stdout integration tests\n\nI falsely assumed an issue in the signal handler itself because the internal check triggered by crashpad_handler forking coincided with DEADLYSIGNAL.\n\nTurns out DEADLYSIGNAL is handled in TSAN like in ASAN, i.e. non-fatal (puh).\n\nThe culprit for crashpad is the fork during initialization, which breaks with TSAN invariants.\n\n* move ringbuffer tests in separate tu\n\n* missed cmake update\n\n* get rid of another unnecessary lock nesting\n\n* clone the DSC when prepping for the envelope header\n\n* clone the DSC when prepping for the envelope header (also in event path)\n\n* dramatically limit scope lock when prepping envelopes.\n\nAlso ensure that the cloned DSC is freed if we don't add it as a header.\n\n* remove unused imports\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-16T17:21:01+02:00",
          "tree_id": "2861324adb9817ab119e8c2571dda42436827416",
          "url": "https://github.com/getsentry/sentry-native/commit/70f38a25655323f2e1e336a8771725c335c41319"
        },
        "date": 1752679493646,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.80989999996018,
            "unit": "ms",
            "extra": "Min 8.547ms\nMax 9.772ms\nMean 8.951ms\nStdDev 0.489ms\nMedian 8.810ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.15840000004664,
            "unit": "ms",
            "extra": "Min 8.475ms\nMax 10.450ms\nMean 9.229ms\nStdDev 0.753ms\nMedian 9.158ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.870399999987967,
            "unit": "ms",
            "extra": "Min 22.394ms\nMax 23.701ms\nMean 23.040ms\nStdDev 0.543ms\nMedian 22.870ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.016399999935856613,
            "unit": "ms",
            "extra": "Min 0.012ms\nMax 0.018ms\nMean 0.015ms\nStdDev 0.003ms\nMedian 0.016ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3689000000122178,
            "unit": "ms",
            "extra": "Min 0.351ms\nMax 0.513ms\nMean 0.400ms\nStdDev 0.066ms\nMedian 0.369ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.892500000044492,
            "unit": "ms",
            "extra": "Min 11.437ms\nMax 13.189ms\nMean 12.246ms\nStdDev 0.808ms\nMedian 11.893ms"
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
          "id": "c3aa571d4eceffcaf4987cde769c92f2bc099753",
          "message": "feat: add `sentry__path_get_mtime()` (#1317)\n\n* feat: add `sentry__path_get_mtime()`\n\n* Apply suggestions from code review\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T16:03:41+02:00",
          "tree_id": "3baeb34b123904e7fdce41b8585a07d9d6e6d617",
          "url": "https://github.com/getsentry/sentry-native/commit/c3aa571d4eceffcaf4987cde769c92f2bc099753"
        },
        "date": 1753452452501,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.19669999996131,
            "unit": "ms",
            "extra": "Min 8.606ms\nMax 10.325ms\nMean 9.218ms\nStdDev 0.690ms\nMedian 9.197ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.041000000024724,
            "unit": "ms",
            "extra": "Min 8.659ms\nMax 10.419ms\nMean 9.201ms\nStdDev 0.705ms\nMedian 9.041ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.890699999995377,
            "unit": "ms",
            "extra": "Min 20.009ms\nMax 25.334ms\nMean 21.668ms\nStdDev 2.172ms\nMedian 20.891ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013299999977789412,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.020ms\nMean 0.014ms\nStdDev 0.003ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.34089999996922415,
            "unit": "ms",
            "extra": "Min 0.305ms\nMax 0.594ms\nMean 0.389ms\nStdDev 0.117ms\nMedian 0.341ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.870400000044356,
            "unit": "ms",
            "extra": "Min 10.543ms\nMax 11.390ms\nMean 10.918ms\nStdDev 0.317ms\nMedian 10.870ms"
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
          "id": "205831fda2ece8c784259f7abac6d297a65474a6",
          "message": "ref: crashpad: pre-generate crash event ID (#1319)\n\n* ref: crashpad: generate event ID on the client side\n\n* Update CHANGELOG.md\n\n* Update src/sentry_value.c\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* fixup\n\n* Update src/sentry_value.h\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-07-25T17:30:53+02:00",
          "tree_id": "2ae22807fa39861d2995772f90de749f8f52b9b0",
          "url": "https://github.com/getsentry/sentry-native/commit/205831fda2ece8c784259f7abac6d297a65474a6"
        },
        "date": 1753457690288,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.02229999999804,
            "unit": "ms",
            "extra": "Min 8.880ms\nMax 9.663ms\nMean 9.181ms\nStdDev 0.324ms\nMedian 9.022ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.457999999995081,
            "unit": "ms",
            "extra": "Min 9.040ms\nMax 9.957ms\nMean 9.432ms\nStdDev 0.353ms\nMedian 9.458ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.856799999990017,
            "unit": "ms",
            "extra": "Min 20.443ms\nMax 22.946ms\nMean 21.225ms\nStdDev 0.999ms\nMedian 20.857ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013100000046506466,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.016ms\nMean 0.013ms\nStdDev 0.003ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.37040000000843065,
            "unit": "ms",
            "extra": "Min 0.315ms\nMax 0.396ms\nMean 0.365ms\nStdDev 0.030ms\nMedian 0.370ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.158099999988735,
            "unit": "ms",
            "extra": "Min 10.882ms\nMax 12.209ms\nMean 11.373ms\nStdDev 0.544ms\nMedian 11.158ms"
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
          "id": "1939e5d2b1636fce6320208a451b51a41aadcc47",
          "message": "test: suppress valgrind for `SENTRY_TEST(task_queue)` (#1323)",
          "timestamp": "2025-07-26T08:43:59-04:00",
          "tree_id": "f780ac08a01bbf8976d4039c54192f7f6d697977",
          "url": "https://github.com/getsentry/sentry-native/commit/1939e5d2b1636fce6320208a451b51a41aadcc47"
        },
        "date": 1753534081808,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.995599999911065,
            "unit": "ms",
            "extra": "Min 9.391ms\nMax 10.317ms\nMean 9.854ms\nStdDev 0.380ms\nMedian 9.996ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.999099999959071,
            "unit": "ms",
            "extra": "Min 9.753ms\nMax 10.584ms\nMean 10.120ms\nStdDev 0.337ms\nMedian 9.999ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.542599999951562,
            "unit": "ms",
            "extra": "Min 21.731ms\nMax 34.365ms\nMean 25.321ms\nStdDev 5.324ms\nMedian 22.543ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011699999959091656,
            "unit": "ms",
            "extra": "Min 0.011ms\nMax 0.013ms\nMean 0.012ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3819000000930828,
            "unit": "ms",
            "extra": "Min 0.322ms\nMax 0.411ms\nMean 0.377ms\nStdDev 0.035ms\nMedian 0.382ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.779100000012477,
            "unit": "ms",
            "extra": "Min 11.063ms\nMax 17.982ms\nMean 12.825ms\nStdDev 2.901ms\nMedian 11.779ms"
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
          "id": "2ac959eff4f76935fcdd4c81dd1cca1056c01f77",
          "message": "feat: read and capture envelopes (#1320)\n\n* feat: raw envelope headers\n\n* Update CHANGELOG.md\n\n* move parsing to sentry_envelope_read_from_file()\n\n* cast\n\n* feat: parse whole envelope\n\n* Fix ClangCL warnings\n\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(765,53): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(786,62): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\nD:\\a\\sentry-native\\sentry-native\\src\\sentry_envelope.c(803,61): error : implicit conversion changes signedness: 'long long' to 'unsigned long long' [-Werror,-Wsign-conversion] [C:\\Users\\runneradmin\\AppData\\Local\\Temp\\pytest-of-runneradmin\\pytest-0\\cmake0\\sentry.vcxproj]\n\n* revert unnecessary changes\n\n* Wide-string variant for Windows\n\n* Improve tests\n\n* size_t payload_len\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>\n\n* sentry_envelope_deserialize + tests\n\n* validate headers\n\n* don't advance ptr past the buffer even if it's never used\n\nmight make cursor satisfied?\n\n* sentry_capture_envelope: add extra null check\n\n* sentry_envelope_get_header_n: add null check\n\n* prevent overflow\n\n* add TODO comments for sentry_value_as_uint64 + SIZE_MAX\n\n---------\n\nCo-authored-by: Ivan Dlugos <6349682+vaind@users.noreply.github.com>",
          "timestamp": "2025-07-29T07:54:02+02:00",
          "tree_id": "358171b00995b08d09eb24f60a2f2080cb3bbcb8",
          "url": "https://github.com/getsentry/sentry-native/commit/2ac959eff4f76935fcdd4c81dd1cca1056c01f77"
        },
        "date": 1753768682151,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.037300000012237,
            "unit": "ms",
            "extra": "Min 7.849ms\nMax 8.202ms\nMean 8.052ms\nStdDev 0.141ms\nMedian 8.037ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.597100000031332,
            "unit": "ms",
            "extra": "Min 8.342ms\nMax 9.241ms\nMean 8.690ms\nStdDev 0.333ms\nMedian 8.597ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.928300000003674,
            "unit": "ms",
            "extra": "Min 19.282ms\nMax 21.164ms\nMean 20.040ms\nStdDev 0.708ms\nMedian 19.928ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00969999996414117,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.012ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3823000000124921,
            "unit": "ms",
            "extra": "Min 0.312ms\nMax 0.410ms\nMean 0.366ms\nStdDev 0.047ms\nMedian 0.382ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.674400000027617,
            "unit": "ms",
            "extra": "Min 10.469ms\nMax 11.280ms\nMean 10.771ms\nStdDev 0.331ms\nMedian 10.674ms"
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
          "id": "93d69f4172c530d8e4ac581df2e80d97e24c7398",
          "message": "feat: add `sentry__process_spawn()` (#1318)\n\n* feat: add `sentry__process_spawn()`\n\n* stabilize SENTRY_TEST(process_spawn)\n\n* add null checks for executable->path\n\n* test: drop sentry__process_spawn return value check\n\njust to see if the CI/Valgrind is happy with the rest\n\n* sentry__process_spawn: remove falsy return value\n\nas claude put it\n\n> This is inherent to fully detached processes - you can't reliably detect launch success without IPC\n\n* avoid path lookup for security reasons\n\n* add note that arguments are not sanitized\n\n* windows: quote args if necessary",
          "timestamp": "2025-07-30T13:30:36+02:00",
          "tree_id": "6593bb13efcc69f02e077905dd2d47340e30e417",
          "url": "https://github.com/getsentry/sentry-native/commit/93d69f4172c530d8e4ac581df2e80d97e24c7398"
        },
        "date": 1753875321602,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.731799999997293,
            "unit": "ms",
            "extra": "Min 9.684ms\nMax 10.468ms\nMean 9.966ms\nStdDev 0.367ms\nMedian 9.732ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 10.496400000022277,
            "unit": "ms",
            "extra": "Min 9.646ms\nMax 11.741ms\nMean 10.488ms\nStdDev 0.823ms\nMedian 10.496ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.856400000013764,
            "unit": "ms",
            "extra": "Min 19.937ms\nMax 21.124ms\nMean 20.617ms\nStdDev 0.485ms\nMedian 20.856ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.011700000015935075,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.012ms\nMean 0.011ms\nStdDev 0.001ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.36670000002914094,
            "unit": "ms",
            "extra": "Min 0.354ms\nMax 0.441ms\nMean 0.392ms\nStdDev 0.043ms\nMedian 0.367ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.064000000033047,
            "unit": "ms",
            "extra": "Min 10.950ms\nMax 11.725ms\nMean 11.201ms\nStdDev 0.326ms\nMedian 11.064ms"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "email": "markus.hintersteiner@sentry.io",
            "name": "Markus Hintersteiner",
            "username": "markushi"
          },
          "committer": {
            "email": "noreply@github.com",
            "name": "GitHub",
            "username": "web-flow"
          },
          "distinct": true,
          "id": "e2c2c0523ba9d14dac0c969216594c975a2a7709",
          "message": "fix(ci): Fix missing debug symbol upload for craft (#1327)\n\nThe `libsentry.so` and `libsentry-android.so` files where not uploaded to the git release sha, causing craft to be unable to find and upload these symbols to our symbol server.\n\n#skip-changelog",
          "timestamp": "2025-07-31T08:54:57+02:00",
          "tree_id": "a844f7ca37cb51840d220a0fbee724d848064acf",
          "url": "https://github.com/getsentry/sentry-native/commit/e2c2c0523ba9d14dac0c969216594c975a2a7709"
        },
        "date": 1753945135764,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.663699999999608,
            "unit": "ms",
            "extra": "Min 8.252ms\nMax 8.905ms\nMean 8.572ms\nStdDev 0.283ms\nMedian 8.664ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.612700000014684,
            "unit": "ms",
            "extra": "Min 8.319ms\nMax 8.938ms\nMean 8.644ms\nStdDev 0.260ms\nMedian 8.613ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.59200000007877,
            "unit": "ms",
            "extra": "Min 19.675ms\nMax 22.118ms\nMean 20.872ms\nStdDev 0.942ms\nMedian 20.592ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.015200000007098424,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.048ms\nMean 0.022ms\nStdDev 0.015ms\nMedian 0.015ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.34110000001419394,
            "unit": "ms",
            "extra": "Min 0.324ms\nMax 0.381ms\nMean 0.349ms\nStdDev 0.022ms\nMedian 0.341ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.556400000041322,
            "unit": "ms",
            "extra": "Min 10.192ms\nMax 11.600ms\nMean 10.676ms\nStdDev 0.541ms\nMedian 10.556ms"
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
          "id": "98b38912bc3ed32e4b014cac72cd8f2aff497608",
          "message": "feat: add `(u)int64 sentry_value_t` type (#1326)\n\n* add (u)int64 sentry_value_t type\n\n* add value_to_msgpack missing switch cases\n\n* remove undefined behavior test (C99 6.3.1.4)\n\n* avoid Windows sized integer name collision\n\n* cleanup & apply code review feedback\n\n* more cleanup & remove type coercion\n\n* correctly parse (u)int64 in sentry__value_from_json\n\n* CHANGELOG.md\n\n* always send uint64 as string\n\n* simplify value_from_json integer parsing\n\n* use stdint macros instead of literals\n\n* fixed value_stringify + added unit test\n\n* Apply suggestions from code review\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>\n\n* free stringify memory + cleanup test\n\n* stringify NAN / INFINITY test with contains check\n\n* add explanation/link for writing uint64 as string\n\n* Fix macro not using parameter\n\n* update ndk gradle dependencies\n\n---------\n\nCo-authored-by: J-P Nurmi <jpnurmi@gmail.com>",
          "timestamp": "2025-07-31T12:37:10+02:00",
          "tree_id": "783406732616d145a8f2eb92a5a95fc86cfd6c7f",
          "url": "https://github.com/getsentry/sentry-native/commit/98b38912bc3ed32e4b014cac72cd8f2aff497608"
        },
        "date": 1753958473320,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.831700000006549,
            "unit": "ms",
            "extra": "Min 8.955ms\nMax 10.180ms\nMean 9.736ms\nStdDev 0.500ms\nMedian 9.832ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.751199999982418,
            "unit": "ms",
            "extra": "Min 9.579ms\nMax 17.844ms\nMean 11.439ms\nStdDev 3.590ms\nMedian 9.751ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 21.317199999998593,
            "unit": "ms",
            "extra": "Min 20.936ms\nMax 24.455ms\nMean 21.890ms\nStdDev 1.447ms\nMedian 21.317ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.016700000060154707,
            "unit": "ms",
            "extra": "Min 0.014ms\nMax 0.017ms\nMean 0.016ms\nStdDev 0.001ms\nMedian 0.017ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.33140000005005277,
            "unit": "ms",
            "extra": "Min 0.303ms\nMax 0.453ms\nMean 0.358ms\nStdDev 0.063ms\nMedian 0.331ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.11470000000736,
            "unit": "ms",
            "extra": "Min 10.552ms\nMax 12.875ms\nMean 11.414ms\nStdDev 0.888ms\nMedian 11.115ms"
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
          "id": "79bf2dbc59b2cf149913f819deb4777707e193cd",
          "message": "ref: use SIZE_MAX in sentry_envelope_deserialize (#1328)\n\n* ref: use SIZE_MAX in sentry_envelope_deserialize\n\n* restore payload_len < 0 check\n\n* fix length parsing vuln\n\n* Update src/sentry_envelope.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-01T13:01:26+02:00",
          "tree_id": "fa61a69295ded8986cd7509f2eed29058bb5cb08",
          "url": "https://github.com/getsentry/sentry-native/commit/79bf2dbc59b2cf149913f819deb4777707e193cd"
        },
        "date": 1754046345961,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.957799999995132,
            "unit": "ms",
            "extra": "Min 9.595ms\nMax 12.417ms\nMean 10.360ms\nStdDev 1.164ms\nMedian 9.958ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.66760000000022,
            "unit": "ms",
            "extra": "Min 9.115ms\nMax 10.323ms\nMean 9.756ms\nStdDev 0.500ms\nMedian 9.668ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 22.982800000022507,
            "unit": "ms",
            "extra": "Min 22.162ms\nMax 45.239ms\nMean 27.476ms\nStdDev 9.979ms\nMedian 22.983ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.012099999935344385,
            "unit": "ms",
            "extra": "Min 0.010ms\nMax 0.015ms\nMean 0.012ms\nStdDev 0.002ms\nMedian 0.012ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3960999999890191,
            "unit": "ms",
            "extra": "Min 0.308ms\nMax 0.405ms\nMean 0.370ms\nStdDev 0.044ms\nMedian 0.396ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.792399999990266,
            "unit": "ms",
            "extra": "Min 10.983ms\nMax 15.645ms\nMean 12.491ms\nStdDev 1.851ms\nMedian 11.792ms"
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
          "id": "aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11",
          "message": "fix: add wildcard entries to valgrind suppression for pthread stack leak (#1330)",
          "timestamp": "2025-08-05T10:02:56+02:00",
          "tree_id": "36e09335b55bdfbcb825b1513e776d9972ae5283",
          "url": "https://github.com/getsentry/sentry-native/commit/aa0dd4b5707af1134cbd3cab8730e9e4fcdfcb11"
        },
        "date": 1754381206633,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.435399999939364,
            "unit": "ms",
            "extra": "Min 7.976ms\nMax 10.076ms\nMean 8.736ms\nStdDev 0.847ms\nMedian 8.435ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.907500000053915,
            "unit": "ms",
            "extra": "Min 9.359ms\nMax 11.124ms\nMean 10.098ms\nStdDev 0.660ms\nMedian 9.908ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 20.858200000020588,
            "unit": "ms",
            "extra": "Min 20.269ms\nMax 23.226ms\nMean 21.222ms\nStdDev 1.174ms\nMedian 20.858ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.00969999996414117,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.010ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.34989999994650134,
            "unit": "ms",
            "extra": "Min 0.305ms\nMax 0.393ms\nMean 0.345ms\nStdDev 0.037ms\nMedian 0.350ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.599700000057055,
            "unit": "ms",
            "extra": "Min 10.084ms\nMax 10.882ms\nMean 10.554ms\nStdDev 0.294ms\nMedian 10.600ms"
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
          "id": "32e09d06b5184e562b8f2762400bf3e9e5e806f3",
          "message": "chore: Change issue template type to label (#1332)\n\n* Update issue template\n\nWe no longer use Issue Type, but Labels instead.\n\n* Update .github/ISSUE_TEMPLATE/bug_report.md\n\n---------\n\nCo-authored-by: Karl Heinz Struggl <kahest@users.noreply.github.com>",
          "timestamp": "2025-08-07T10:29:17+02:00",
          "tree_id": "ac35ea7b9bca448a239326161fcc223f8bce0c32",
          "url": "https://github.com/getsentry/sentry-native/commit/32e09d06b5184e562b8f2762400bf3e9e5e806f3"
        },
        "date": 1754555589117,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 8.522499999969568,
            "unit": "ms",
            "extra": "Min 8.186ms\nMax 8.883ms\nMean 8.488ms\nStdDev 0.277ms\nMedian 8.522ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 8.813699999961955,
            "unit": "ms",
            "extra": "Min 8.476ms\nMax 9.239ms\nMean 8.813ms\nStdDev 0.304ms\nMedian 8.814ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 19.565099999965696,
            "unit": "ms",
            "extra": "Min 19.375ms\nMax 21.101ms\nMean 19.867ms\nStdDev 0.705ms\nMedian 19.565ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.01059999999597494,
            "unit": "ms",
            "extra": "Min 0.009ms\nMax 0.011ms\nMean 0.010ms\nStdDev 0.001ms\nMedian 0.011ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3606000000218046,
            "unit": "ms",
            "extra": "Min 0.322ms\nMax 0.436ms\nMean 0.367ms\nStdDev 0.043ms\nMedian 0.361ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.356500000000324,
            "unit": "ms",
            "extra": "Min 10.298ms\nMax 10.507ms\nMean 10.374ms\nStdDev 0.086ms\nMedian 10.357ms"
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
          "id": "305dec3449cf43975d7987510382f7149c435115",
          "message": "chore: move xbox toolchain to sentry-xbox repository (#1329)\n\n* chore: move xbox toolchain to sentry-xbox repository\n\n* update platform support docs\n\n* chore: changelog\n\n* Update CHANGELOG.md\n\n* cleanup\n\n* keep sentry.native.xbox as sdk name\n\n* restore xbox_build.md\n\n* fixup: restore sentry.h properly",
          "timestamp": "2025-08-07T12:47:11+02:00",
          "tree_id": "80b06a68781990309fa25116c73e2f3092a1aee8",
          "url": "https://github.com/getsentry/sentry-native/commit/305dec3449cf43975d7987510382f7149c435115"
        },
        "date": 1754563869919,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 9.055099999955019,
            "unit": "ms",
            "extra": "Min 8.095ms\nMax 9.934ms\nMean 8.897ms\nStdDev 0.733ms\nMedian 9.055ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.11750000000211,
            "unit": "ms",
            "extra": "Min 8.727ms\nMax 12.132ms\nMean 9.973ms\nStdDev 1.514ms\nMedian 9.118ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 24.213900000063404,
            "unit": "ms",
            "extra": "Min 21.276ms\nMax 39.112ms\nMean 26.680ms\nStdDev 7.388ms\nMedian 24.214ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013699999954042141,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.017ms\nMean 0.014ms\nStdDev 0.002ms\nMedian 0.014ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.3504000000020824,
            "unit": "ms",
            "extra": "Min 0.316ms\nMax 0.407ms\nMean 0.355ms\nStdDev 0.039ms\nMedian 0.350ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 10.533599999916987,
            "unit": "ms",
            "extra": "Min 10.260ms\nMax 14.178ms\nMean 11.243ms\nStdDev 1.661ms\nMedian 10.534ms"
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
          "id": "6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee",
          "message": "feat: manage trace life-cycle (#1270)\n\n* feat: manage trace duration\n\n* format after web-editor merge\n\n* clean up regenerate_trace() test after merge\n\n* prevent warning-as-error\n\n* add changelog\n\n* ensure trace_managed is turned off when regenerating a trace\n\n* update breaking changes\n\n* test that transaction no longer act as trace boundaries after a user regenerates a trace\n\n* highlight the topic of automatic trace boundaries in the docs for `sentry_set_trace()` and `sentry_regenerate_trace()`\n\n* Update tests/unit/test_tracing.c\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>\n\n* remove duplicate includes in sentry_core.c\n\n* tighten regenerate part of the test against the trace generated during the previous transaction\n\n---------\n\nCo-authored-by: JoshuaMoelans <60878493+JoshuaMoelans@users.noreply.github.com>",
          "timestamp": "2025-08-08T12:24:46+02:00",
          "tree_id": "13cad9add2e8e83d579991f58deca96160144236",
          "url": "https://github.com/getsentry/sentry-native/commit/6466da9d27bde1f4c9fe5c01aeaf74f0b3f837ee"
        },
        "date": 1754648922680,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 10.117299999933493,
            "unit": "ms",
            "extra": "Min 9.482ms\nMax 10.649ms\nMean 10.095ms\nStdDev 0.509ms\nMedian 10.117ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 9.752199999979894,
            "unit": "ms",
            "extra": "Min 9.205ms\nMax 9.960ms\nMean 9.620ms\nStdDev 0.372ms\nMedian 9.752ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 23.449599999935344,
            "unit": "ms",
            "extra": "Min 21.972ms\nMax 26.588ms\nMean 23.600ms\nStdDev 1.820ms\nMedian 23.450ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.015599999983351154,
            "unit": "ms",
            "extra": "Min 0.015ms\nMax 0.021ms\nMean 0.017ms\nStdDev 0.002ms\nMedian 0.016ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.39270000002034067,
            "unit": "ms",
            "extra": "Min 0.343ms\nMax 0.504ms\nMean 0.407ms\nStdDev 0.060ms\nMedian 0.393ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 11.7753999999195,
            "unit": "ms",
            "extra": "Min 11.426ms\nMax 11.934ms\nMean 11.719ms\nStdDev 0.194ms\nMedian 11.775ms"
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
          "id": "d5c34db05c6d8cca302c2dface5063b0488df5be",
          "message": "craft: only include zip artifacts for github target (#1333)\n\n#skip-changelog",
          "timestamp": "2025-08-08T13:53:16+02:00",
          "tree_id": "bbe859661565e9a10a808263d75f3a5c6cf8aecf",
          "url": "https://github.com/getsentry/sentry-native/commit/d5c34db05c6d8cca302c2dface5063b0488df5be"
        },
        "date": 1754654249281,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "SDK init (inproc)",
            "value": 10.813100000007125,
            "unit": "ms",
            "extra": "Min 9.900ms\nMax 11.892ms\nMean 10.894ms\nStdDev 0.832ms\nMedian 10.813ms"
          },
          {
            "name": "SDK init (breakpad)",
            "value": 11.625100000003386,
            "unit": "ms",
            "extra": "Min 11.292ms\nMax 17.352ms\nMean 12.750ms\nStdDev 2.581ms\nMedian 11.625ms"
          },
          {
            "name": "SDK init (crashpad)",
            "value": 23.68240000004107,
            "unit": "ms",
            "extra": "Min 22.999ms\nMax 66.004ms\nMean 32.054ms\nStdDev 18.983ms\nMedian 23.682ms"
          },
          {
            "name": "Backend startup (inproc)",
            "value": 0.013099999989663047,
            "unit": "ms",
            "extra": "Min 0.013ms\nMax 0.015ms\nMean 0.013ms\nStdDev 0.001ms\nMedian 0.013ms"
          },
          {
            "name": "Backend startup (breakpad)",
            "value": 0.4271000000244385,
            "unit": "ms",
            "extra": "Min 0.375ms\nMax 0.627ms\nMean 0.453ms\nStdDev 0.101ms\nMedian 0.427ms"
          },
          {
            "name": "Backend startup (crashpad)",
            "value": 13.581900000019687,
            "unit": "ms",
            "extra": "Min 12.536ms\nMax 15.333ms\nMean 13.860ms\nStdDev 1.056ms\nMedian 13.582ms"
          }
        ]
      }
    ]
  }
}