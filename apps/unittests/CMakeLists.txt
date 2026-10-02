era_app(unittests
    DEPENDS physics dynx gtest gtest_main)

add_test(NAME EngineUnittests COMMAND unittests)
