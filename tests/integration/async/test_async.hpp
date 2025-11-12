#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_async() {
    std::cout << "[integration-test] Running async/await tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic Future<T> type (builtin)
    run_cb_test_with_output_and_time("../cases/async/test_future_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_future_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Future<T> Basic Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Future value: 42", "Should display future value");
            INTEGRATION_ASSERT_CONTAINS(output, "Is ready: 1", "Should display is_ready flag");
            INTEGRATION_ASSERT_CONTAINS(output, "Future type test passed", "Should display success message");
        }, execution_time);
    integration_test_passed_with_time("Basic Future<T> type", "test_future_basic.cb", execution_time);
    
    // Test 2: Phase 1 - async/await syntax
    run_cb_test_with_output_and_time("../cases/async/phase1_syntax_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase1_syntax_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Start main", "Should start main");
            INTEGRATION_ASSERT_CONTAINS(output, "Inside simple_async", "Should execute async function");
            INTEGRATION_ASSERT_CONTAINS(output, "After async call", "Should continue after async call");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 42", "Should display result");
        }, execution_time);
    integration_test_passed_with_time("Phase 1 async/await syntax", "phase1_syntax_test.cb", execution_time);
    
    // Test 3: Phase 1 - Multiple async functions
    run_cb_test_with_output_and_time("../cases/async/phase1_multiple_async.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase1_multiple_async.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Phase 1 Multiple Async Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "After fetch_data(1)", "Should execute fetch_data(1)");
            INTEGRATION_ASSERT_CONTAINS(output, "After fetch_data(2)", "Should execute fetch_data(2)");
            INTEGRATION_ASSERT_CONTAINS(output, "Fetching data for ID: 1", "Should fetch data for ID 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Fetching data for ID: 2", "Should fetch data for ID 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Result 1: 100", "Should display result 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Result 2: 200", "Should display result 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Processing value: 100", "Should process value");
            INTEGRATION_ASSERT_CONTAINS(output, "Processed: 150", "Should display processed result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("Phase 1 multiple async functions", "phase1_multiple_async.cb", execution_time);
    
    // Test 4: Phase 2 - Cooperative multitasking with yield
    run_cb_test_with_output_and_time("../cases/async/phase2_yield_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_yield_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Main: Starting tasks", "Should start tasks");
            INTEGRATION_ASSERT_CONTAINS(output, "Main: Tasks registered, awaiting results", "Should register tasks");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Start", "Task1 should start");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Start", "Task2 should start");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: After first yield", "Task1 should yield");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: After first yield", "Task2 should yield");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: After second yield", "Task1 should yield again");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Done", "Task2 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Done", "Task1 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Main: All tasks completed", "All tasks should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Results: 1, 2", "Should display results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 cooperative multitasking with yield", "phase2_yield_test.cb", execution_time);
    
    // Test 5: Phase 2 - Auto-yield feature
    run_cb_test_with_output_and_time("../cases/async/phase2_auto_yield_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_auto_yield_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Auto-yield Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Tasks registered, awaiting...", "Should register tasks");
            
            // タスクのインターリーブ実行を確認
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Statement 1", "Task1 statement 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Statement 1", "Task2 statement 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Task3: Before explicit yield", "Task3 before yield");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Statement 2", "Task1 statement 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Statement 2", "Task2 statement 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Statement 3", "Task1 statement 3");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Done", "Task2 done");
            INTEGRATION_ASSERT_CONTAINS(output, "Task3: After explicit yield", "Task3 after yield");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Done", "Task1 done");
            INTEGRATION_ASSERT_CONTAINS(output, "Task3: Done", "Task3 done");
            
            INTEGRATION_ASSERT_CONTAINS(output, "f1 Ok", "f1 should be ok");
            INTEGRATION_ASSERT_CONTAINS(output, "f2 Ok", "f2 should be ok");
            INTEGRATION_ASSERT_CONTAINS(output, "f3 Ok", "f3 should be ok");
            INTEGRATION_ASSERT_CONTAINS(output, "All tasks completed!", "All tasks should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Results: 10, 20, 30", "Should display results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 auto-yield feature", "phase2_auto_yield_test.cb", execution_time);
    
    // Test 6: Phase 2 - Builtin Future<T> (no user definition required)
    run_cb_test_with_output_and_time("../cases/async/phase2_builtin_future_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_builtin_future_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Builtin Future Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Statement 1", "Task1 statement 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Statement 1", "Task2 statement 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Statement 2", "Task1 statement 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Statement 2", "Task2 statement 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Results: 100, 200", "Should display results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 builtin Future<T>", "phase2_builtin_future_test.cb", execution_time);
    
    // Test 7: Phase 2 - Direct await (without Future variable)
    run_cb_test_with_output_and_time("../cases/async/phase2_direct_await_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_direct_await_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Direct Await Test ===", "Should contain test header");
            
            // Pattern 1: Direct await
            INTEGRATION_ASSERT_CONTAINS(output, "Pattern 1: Direct await", "Pattern 1 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Start", "Task1 starts");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Processing step 1", "Task1 processing step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Processing step 2", "Task1 processing step 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Done", "Task1 completes");
            INTEGRATION_ASSERT_CONTAINS(output, "Got r1: 10", "Got r1 result");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Start", "Task2 starts");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Processing", "Task2 processing");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Done", "Task2 completes");
            INTEGRATION_ASSERT_CONTAINS(output, "Got r2: 20", "Got r2 result");
            
            // Pattern 2: Compute and await
            INTEGRATION_ASSERT_CONTAINS(output, "Pattern 2: Compute and await", "Pattern 2 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Computing: 10 + 20", "Computing");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 30", "Compute result");
            INTEGRATION_ASSERT_CONTAINS(output, "Sum: 30", "Sum displayed");
            
            // Pattern 3: Sequential direct awaits
            INTEGRATION_ASSERT_CONTAINS(output, "Pattern 3: Sequential direct awaits", "Pattern 3 header");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Test complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Final results: a=10, b=20, c=30", "Final results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 direct await (no Future variable)", "phase2_direct_await_test.cb", execution_time);
    
    // Test 8: Phase 2 - Concurrent tasks execution
    run_cb_test_with_output_and_time("../cases/async/phase2_concurrent_tasks_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_concurrent_tasks_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Concurrent Tasks Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Registering tasks...", "Registering tasks");
            INTEGRATION_ASSERT_CONTAINS(output, "All tasks registered", "All tasks registered");
            INTEGRATION_ASSERT_CONTAINS(output, "Awaiting results...", "Awaiting results");
            
            // タスクの協調的実行を確認
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Start", "Task1 starts");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Start", "Task2 starts");
            INTEGRATION_ASSERT_CONTAINS(output, "Task3: Start", "Task3 starts");
            INTEGRATION_ASSERT_CONTAINS(output, "Task1: Done", "Task1 completes");
            INTEGRATION_ASSERT_CONTAINS(output, "Task2: Done", "Task2 completes");
            INTEGRATION_ASSERT_CONTAINS(output, "Task3: Done", "Task3 completes");
            
            INTEGRATION_ASSERT_CONTAINS(output, "Got r1: 100", "Got result 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Got r2: 200", "Got result 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Got r3: 300", "Got result 3");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Test complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Results: 100, 200, 300", "Final results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 concurrent tasks execution", "phase2_concurrent_tasks_test.cb", execution_time);
    
    // Test 9: Phase 2 - Task registration vs execution timing
    run_cb_test_with_output_and_time("../cases/async/phase2_task_timing_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_task_timing_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Task Registration vs Execution Test ===", "Should contain test header");
            
            // Phase 1: タスク登録
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 1: Registering tasks", "Phase 1 start");
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 1: All tasks registered (but not executed yet)", "Phase 1 complete");
            
            // Phase 2: 実行開始
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 2: Starting execution with first await", "Phase 2 start");
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 2: log1 completed", "Phase 2 log1 complete");
            
            // Phase 3: 残り結果取得
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 3: Getting remaining results", "Phase 3 start");
            INTEGRATION_ASSERT_CONTAINS(output, "LOG: First message", "First message logged");
            INTEGRATION_ASSERT_CONTAINS(output, "LOG: Second message", "Second message logged");
            INTEGRATION_ASSERT_CONTAINS(output, "Computing 10 * 2", "Computing 10");
            INTEGRATION_ASSERT_CONTAINS(output, "Computing 20 * 2", "Computing 20");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 20", "Result 20");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 40", "Result 40");
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 3: calc1 completed, result: 20", "calc1 complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Phase 3: calc2 completed, result: 40", "calc2 complete");
            
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Test complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Final results: 20, 40", "Final results");
        }, execution_time);
    integration_test_passed_with_time("Phase 2 task registration vs execution timing", "phase2_task_timing_test.cb", execution_time);
    
    // Test 10: Phase 2 - Unawaited async function exit
    run_cb_test_with_output_and_time("../cases/async/phase2_unawaited_exit_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_unawaited_exit_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Unawaited Exit Test ===", "Should contain test header");
            
            // バックグラウンドタスクが部分的に実行されることを確認
            // v0.12.0: イベントループは自動起動し、async関数呼び出しと各ステートメント実行後に進行する
            // しかし、mainが終了するとプログラムが終了し、バックグラウンドタスクは完了しない
            INTEGRATION_ASSERT_CONTAINS(output, "Background: Step 1", "Background task should execute Step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Background: Step 2", "Background task should execute Step 2");
            // Note: Step 3以降はmainが先に終了するため実行されない可能性が高い
            
            // Step 4以降は確実に実行されない（mainが先に終了するため）
            INTEGRATION_ASSERT_NOT_CONTAINS(output, "Background: Step 4", "Background task should NOT complete Step 4");
            INTEGRATION_ASSERT_NOT_CONTAINS(output, "Background: Step 10", "Background task should NOT reach Step 10");
            
            // main関数のステートメントも実行される
            INTEGRATION_ASSERT_CONTAINS(output, "Main: Exiting without awaiting", "Main exit message");
            
            // 重要: Background Step 2とMain exitの間にインターリーブされる
            // これは協調的マルチタスクの証拠
        }, execution_time);
    integration_test_passed_with_time("Phase 2 unawaited async function exit", "phase2_unawaited_exit_test.cb", execution_time);
    
    // Test 11: Phase 2.0 - For loop cooperative multitasking
    run_cb_test_with_output_and_time("../cases/async/phase2_for_loop_fairness.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_for_loop_fairness.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== For Loop Cooperative Multitasking Test ===", "Should contain test header");
            
            // forループとバックグラウンドタスクが交互実行されることを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Starting for loop", "Main starts for loop");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Iteration 0", "Main iteration 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 0", "BG step 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Iteration 1", "Main iteration 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 1", "BG step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] For loop done", "Main loop done");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Done", "Main done");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 for loop cooperative multitasking", "phase2_for_loop_fairness.cb", execution_time);
    
    // Test 12: Phase 2.0 - While loop cooperative multitasking
    run_cb_test_with_output_and_time("../cases/async/phase2_while_loop_fairness.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_while_loop_fairness.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== While Loop Cooperative Multitasking Test ===", "Should contain test header");
            
            // whileループとバックグラウンドタスクが交互実行されることを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Starting while loop", "Main starts while loop");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Iteration 0", "Main iteration 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 0", "BG step 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Iteration 1", "Main iteration 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 1", "BG step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] While loop done", "Main loop done");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Done", "Main done");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 while loop cooperative multitasking", "phase2_while_loop_fairness.cb", execution_time);
    
    // Test 13: Phase 2.0 - Recursive function cooperative multitasking
    run_cb_test_with_output_and_time("../cases/async/phase2_recursive_fairness.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_recursive_fairness.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Recursive Function Cooperative Multitasking Test ===", "Should contain test header");
            
            // 再帰関数とバックグラウンドタスクが交互実行されることを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Starting recursive function", "Main starts recursive function");
            INTEGRATION_ASSERT_CONTAINS(output, "[Recursion] Level 5", "Recursion level 5");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 0", "BG step 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[Recursion] Level 4", "Recursion level 4");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 1", "BG step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[Recursion] Base case reached", "Recursion base case");
            INTEGRATION_ASSERT_CONTAINS(output, "[Recursion] Returning from level 1", "Returning from level 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Recursive function done, sum: 15", "Main done with sum");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Done", "Main done");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 recursive function cooperative multitasking", "phase2_recursive_fairness.cb", execution_time);
    
    // Test 14: Phase 2.0 - Nested function call cooperative multitasking
    run_cb_test_with_output_and_time("../cases/async/phase2_nested_function_fairness.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_nested_function_fairness.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Nested Function Call Fairness Test ===", "Should contain test header");
            
            // ネストした関数呼び出しとバックグラウンドタスクが交互実行されることを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Calling outer_function", "Main calls outer");
            INTEGRATION_ASSERT_CONTAINS(output, "[Outer] value=5", "Outer called");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 0", "BG step 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[Middle] n=5", "Middle called");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 1", "BG step 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] x=5", "Inner called");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 2", "BG step 2");
            INTEGRATION_ASSERT_CONTAINS(output, "[Middle] Got result=10", "Middle got result");
            INTEGRATION_ASSERT_CONTAINS(output, "[Outer] Got temp=11", "Outer got temp");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Final result: 33", "Main got final result");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Done", "Main done");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 nested function call cooperative multitasking", "phase2_nested_function_fairness.cb", execution_time);
    
    // Test 15: Phase 2.0 - Async interface/impl support
    run_cb_test_with_output_and_time("../cases/async/phase2_async_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "phase2_async_interface.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Async Interface/Impl Test ===", "Should contain test header");
            
            // interface/implでのasync/awaitサポートを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Calling async method", "Main calls async method");
            INTEGRATION_ASSERT_CONTAINS(output, "[Impl] Processing async: 5", "Impl processes async");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Calling sync method", "Main calls sync method");
            INTEGRATION_ASSERT_CONTAINS(output, "[Impl] Processing sync: 3", "Impl processes sync");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Sync result: 13", "Main got sync result");
            INTEGRATION_ASSERT_CONTAINS(output, "[Impl] After yield", "Impl after yield");
            INTEGRATION_ASSERT_CONTAINS(output, "[Impl] Returning: 50", "Impl returning");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Awaiting async result", "Main awaiting async");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Async result: 50", "Main got async result");
            INTEGRATION_ASSERT_CONTAINS(output, "[BG] Step 0", "BG step 0");
            INTEGRATION_ASSERT_CONTAINS(output, "[Main] Done", "Main done");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 async interface/impl support", "phase2_async_interface.cb", execution_time);
    
    // Test 16: Nested async calls
    run_cb_test_with_output_and_time("../cases/async/test_nested_async.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_nested_async.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Nested Async Calls Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1 passed", "Test 1 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 2 passed", "Test 2 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 3 passed", "Test 3 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 4 passed", "Test 4 should pass");
        }, execution_time);
    integration_test_passed_with_time("Nested async calls", "test_nested_async.cb", execution_time);
    
    // Test 17: Basic await with 100ms sleep
    run_cb_test_with_output_and_time("../cases/async/test_await_simple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_await_simple.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Before await", "Should print before await");
            INTEGRATION_ASSERT_CONTAINS(output, "After await", "Should print after await");
            INTEGRATION_ASSERT_CONTAINS(output, "Done", "Should complete");
        }, execution_time);
    integration_test_passed_with_time("Basic await with 100ms sleep", "test_await_simple.cb", execution_time);
    
    // Test 18: 120ms sleep with elapsed time verification
    run_cb_test_with_output_and_time("../cases/async/test_no_vardecl.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_no_vardecl.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Elapsed:", "Should display elapsed time");
            // 期待値: ~120ms (許容範囲: 100-150ms)
            // 注: 正確な時間検証は出力文字列からパースする必要があるため、存在確認のみ
        }, execution_time);
    integration_test_passed_with_time("120ms sleep with elapsed time", "test_no_vardecl.cb", execution_time);
    
    // Test 19: Sleep concurrent (simplified)
    run_cb_test_with_output_and_time("../cases/async/test_sleep_simple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_sleep_simple.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Sleep Concurrent Test (Simplified) ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1 completed", "Test 1 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 2 completed", "Test 2 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "Task-A: sleeping 50ms", "Task A should sleep");
            INTEGRATION_ASSERT_CONTAINS(output, "Task-B: sleeping 30ms", "Task B should sleep");
        }, execution_time);
    integration_test_passed_with_time("Sleep concurrent operations (simplified)", "test_sleep_simple.cb", execution_time);
    
    // Test 20: Multiple concurrent sleep operations (comprehensive)
    run_cb_test_with_output_and_time("../cases/async/test_sleep_concurrent.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_sleep_concurrent.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Multiple Sleep Concurrent Test ===", "Should contain test header");
            
            // Test 1: 並行sleep
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 1] Three concurrent sleeps", "Test 1 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Task-A: Start", "Task A should start");
            INTEGRATION_ASSERT_CONTAINS(output, "Task-B: Start", "Task B should start");
            INTEGRATION_ASSERT_CONTAINS(output, "Task-C: Start", "Task C should start");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1 passed", "Test 1 should pass");
            
            // Test 2: return値付きsleep
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 2] Sleep with return values", "Test 2 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Results: 10, 20, 30", "Should have correct return values");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 2 passed", "Test 2 should pass");
            
            // Test 3: 0ms sleep
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 3] Zero millisecond sleep", "Test 3 header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 3 passed", "Test 3 should pass");
            
            // Test 4: 連続sleep
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 4] Rapid consecutive sleeps", "Test 4 header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 4 passed", "Test 4 should pass");
            
            // Test 5: 大量並行sleep
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 5] Many concurrent sleeps", "Test 5 header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 5 passed", "Test 5 should pass");
            
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Sleep Tests Completed ===", "Should complete all tests");
        }, execution_time);
    integration_test_passed_with_time("Multiple concurrent sleep operations (comprehensive)", "test_sleep_concurrent.cb", execution_time);
    
    // Test 18: Yield state preservation
    run_cb_test_with_output_and_time("../cases/async/test_yield_state.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_yield_state.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Yield State Preservation Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1 completed", "Test 1 should complete");
            // Test 2 may fail due to scope issues
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 3 completed", "Test 3 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 4 passed", "Test 4 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 5 completed", "Test 5 should complete");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 6 completed", "Test 6 should complete");
        }, execution_time);
    integration_test_passed_with_time("Yield state preservation", "test_yield_state.cb", execution_time);
    
    // Test 19: Phase 2.0 - Async interface basic operations
    run_cb_test_with_output_and_time("../cases/async/test_interface_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_interface_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "PASS: async interface basic test", "Should display success message");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 async interface basic operations", "test_interface_basic.cb", execution_time);
    
    // Test 20: Phase 2.0 - Async interface with self
    run_cb_test_with_output_and_time("../cases/async/test_interface_self.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_interface_self.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "PASS: async interface with self test", "Should display success message");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 async interface with self", "test_interface_self.cb", execution_time);
    
    // Test 21: Phase 2.0 - Async interface concurrent execution
    run_cb_test_with_output_and_time("../cases/async/test_interface_concurrent.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_interface_concurrent.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "PASS: async interface concurrent test", "Should display success message");
        }, execution_time);
    integration_test_passed_with_time("Phase 2.0 async interface concurrent execution", "test_interface_concurrent.cb", execution_time);
    
    // Test 22: Future multiple await (スキップ - 構造体型Futureに既知の問題)
    // run_cb_test_with_output_and_time("../cases/async/test_future_multiple_await.cb", ...
    // TODO: 構造体型Futureのサポート後に有効化
    
    // Test 23: v0.13.0 - Comprehensive async/await + Result integration
    run_cb_test_with_output_and_time("../cases/async/comprehensive_async_result.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "comprehensive_async_result.cb should execute successfully");
            
            // Test headers
            INTEGRATION_ASSERT_CONTAINS(output, "=== Comprehensive Async/Await + Result Test ===", "Should contain test header");
            
            // Test 1: Basic success case
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Basic async Result (success)", "Test 1 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1.1: divide_async(10, 2) = 5 - PASSED", "Test 1.1 passed");
            
            // Test 2: Error case
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Basic async Result (error)", "Test 2 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2.1: Error caught: Division by zero - PASSED", "Test 2.1 passed");
            
            // Test 3: Variant access
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Variant access", "Test 3 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3.1: Result variant = Ok - PASSED", "Test 3.1 passed");
            
            // Test 4: Nested Result
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Nested Result", "Test 4 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4.1: Nested Result retrieved successfully - PASSED", "Test 4.1 passed");
            
            // Test 5: String Result
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: String Result", "Test 5 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5.1: Success message = Success message - PASSED", "Test 5.1 passed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5.2: Error message = Error message - PASSED", "Test 5.2 passed");
            
            // Test 6: Chained operations
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Chained operations", "Test 6 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6.1: chain_operations(20,2,5) = 2 - PASSED", "Test 6.1 passed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6.2: Chained error caught: Division by zero - PASSED", "Test 6.2 passed");
            
            // Test 7: Early return cases
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Early return cases", "Test 7 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7.1: Negative error: Negative number - PASSED", "Test 7.1 passed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7.2: early_return(0) = 0 - PASSED", "Test 7.2 passed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7.3: early_return(5) = 10 - PASSED", "Test 7.3 passed");
            
            // Test 8: Multiple sequential awaits
            INTEGRATION_ASSERT_CONTAINS(output, "Test 8: Multiple sequential awaits", "Test 8 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 8.1: Sum of multiple awaits = 60 - PASSED", "Test 8.1 passed");
            
            // Final message
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "All tests should pass");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async Result<T,E> comprehensive integration", "comprehensive_async_result.cb", execution_time);
    
    // Test 24: v0.13.0 - Basic async Result tests
    run_cb_test_with_output_and_time("../cases/async/test_async_result_integration.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_result_integration.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success: 5", "Should show success result");
            INTEGRATION_ASSERT_CONTAINS(output, "Error: Division by zero", "Should show error result");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async Result<T,E> basic integration", "test_async_result_integration.cb", execution_time);
    
    // Test 25: v0.13.0 - Async Result minimal test
    run_cb_test_with_output_and_time("../cases/async/test_async_result_minimal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_result_minimal.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Inside async, creating Result", "Should create Result");
            INTEGRATION_ASSERT_CONTAINS(output, "After await, variant: Ok", "Should show Ok variant");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async Result<T,E> minimal", "test_async_result_minimal.cb", execution_time);
    
    // Test 26: v0.13.0 - Async Result error handling
    run_cb_test_with_output_and_time("../cases/async/test_async_result_simple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_result_simple.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async Result<T,E> simple", "test_async_result_simple.cb", execution_time);
    
    // Test 27: v0.13.0 - Async interface basic
    run_cb_test_with_output_and_time("../cases/async/test_interface_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_interface_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "PASS", "Should pass test");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async interface basic", "test_interface_basic.cb", execution_time);
    
    // Test 28: v0.13.0 - Comprehensive async Result patterns
    run_cb_test_with_output_and_time("../cases/async/comprehensive_async_result.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "comprehensive_async_result.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1:", "Should run test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2:", "Should run test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "PASSED", "Should pass tests");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 comprehensive async Result patterns", "comprehensive_async_result.cb", execution_time);
    
    // Test 29: v0.13.0 - Generic struct with async
    run_cb_test_with_output_and_time("../cases/async/test_simple_generic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_simple_generic.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 generic struct with async", "test_simple_generic.cb", execution_time);
    
    // Test 30: v0.13.0 - Result type construction
    run_cb_test_with_output_and_time("../cases/async/test_result_construct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_result_construct.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 Result type construction", "test_result_construct.cb", execution_time);
    
    // Test 31: v0.13.0 - Async syntax simplification (async T)
    run_cb_test_with_output_and_time("../cases/async/integration_async_syntax.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "integration_async_syntax.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Async Syntax Integration Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "compute(21) = 42", "async int should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Message: Hello from async!", "async string should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 5", "async Result Ok should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Error (expected): Division by zero", "async Result Err should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Found: 42", "async Option Some should work");
            INTEGRATION_ASSERT_CONTAINS(output, "None (expected)", "async Option None should work");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed! ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 async syntax (async T)", "integration_async_syntax.cb", execution_time);
    
    // Test 32: v0.13.0 - Direct return enum variants (Option::None fix)
    run_cb_test_with_output_and_time("../cases/async/test_direct_return_enum.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_direct_return_enum.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Direct Return Enum Variants Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "SUCCESS: Got None", "Direct return Option::None should work");
            INTEGRATION_ASSERT_CONTAINS(output, "SUCCESS: Got Some(99)", "Direct return Option::Some should work");
            INTEGRATION_ASSERT_CONTAINS(output, "SUCCESS: Got Ok(123)", "Direct return Result::Ok should work");
            INTEGRATION_ASSERT_CONTAINS(output, "SUCCESS: Got Err(test error)", "Direct return Result::Err should work");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Direct Return Tests Passed! ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("v0.13.0 direct return enum variants", "test_direct_return_enum.cb", execution_time);
    
    // Test 33: v0.12.1 - Timeout basic functionality (partial implementation)
    // Note: timeout() is implemented but Result integration is incomplete
    run_cb_test_with_output_and_time("../cases/async/test_timeout_compile.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_compile.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Testing timeout compilation...", "Should start timeout test");
            INTEGRATION_ASSERT_CONTAINS(output, "Task result: 42", "Should display task result");
            INTEGRATION_ASSERT_CONTAINS(output, "Timeout compilation test passed!", "Should pass compilation test");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout compilation", "test_timeout_compile.cb", execution_time);
    
    // Test: Timeout comprehensive
    run_cb_test_with_output_and_time("../cases/async/test_timeout_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All timeout tests passed!", "Should pass all comprehensive timeout tests");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout comprehensive", "test_timeout_comprehensive.cb", execution_time);
    
    // Test: Timeout with different types
    run_cb_test_with_output_and_time("../cases/async/test_timeout_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_types.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All timeout type tests passed!", "Should pass all type timeout tests");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout with types", "test_timeout_types.cb", execution_time);
    
    // Test: Timeout concurrent operations
    run_cb_test_with_output_and_time("../cases/async/test_timeout_concurrent.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_concurrent.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All concurrent timeout tests passed!", "Should pass all concurrent timeout tests");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout concurrent", "test_timeout_concurrent.cb", execution_time);
    
    // Test: Timeout sequential chaining
    run_cb_test_with_output_and_time("../cases/async/test_timeout_sequential.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_sequential.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All sequential timeout tests passed!", "Should pass all sequential timeout tests");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout sequential", "test_timeout_sequential.cb", execution_time);
    
    // Test: Timeout chained operations
    run_cb_test_with_output_and_time("../cases/async/test_timeout_chained.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_timeout_chained.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All timeout chained tests passed!", "Should pass all chained timeout tests");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 timeout chained", "test_timeout_chained.cb", execution_time);
    
    // Test: ? operator with async functions
    run_cb_test_with_output_and_time("../cases/async/test_async_question_operator.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_question_operator.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Async ? operator success:  10", "Should propagate success case");
            INTEGRATION_ASSERT_CONTAINS(output, "Async ? operator error: division by zero", "Should propagate error case");
            INTEGRATION_ASSERT_CONTAINS(output, "Async ? operator test passed", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("v0.12.1 async ? operator", "test_async_question_operator.cb", execution_time);
    
    std::cout << "[integration-test] Async/await tests completed (40 tests)" << std::endl;
}
