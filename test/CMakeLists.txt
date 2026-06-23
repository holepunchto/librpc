list(APPEND tests
  message
  client
)

foreach(test IN LISTS tests)
  add_executable(${test} ${test}.c)

  target_link_libraries(
    ${test}
    PRIVATE
      rpc
      compact
      utf
  )

  add_test(
    NAME ${test}
    COMMAND ${test}
  )
endforeach()
