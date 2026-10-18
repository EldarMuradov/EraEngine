era_app(unittests
    DEPENDS physics simple_motion_matching dynx gtest gtest_main)

add_test(NAME EngineUnittests COMMAND unittests)

era_format(unittests)
