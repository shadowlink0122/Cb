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
            INTEGRATION_ASSERT_CONTAINS(output, "Background: Step 3", "Background task should execute Step 3");
            
            // Step 4以降は実行されない（mainが先に終了するため）
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
    
    std::cout << "[integration-test] Async/await tests completed" << std::endl;
}
