# Week 3: Event Loop Implementation Plan

**Version**: v0.11.0 Part 2 - Event Loop  
**Status**: 🔵 In Progress  
**Start Date**: 2025-10-27  
**Phase**: Phase 1 Week 1

## Overview

Week 3では、非同期処理の基盤となるEvent Loopを実装します。Week 2で構築したAllocatorインフラストラクチャを活用し、効率的なタスクスケジューリングシステムを構築します。

## Week 3 Goals

### Day 1: Task Structure & Queue (Monday)
**Goal**: タスクの基本構造とキュー実装

**Tasks**:
1. Task構造体の定義
   ```cb
   struct Task {
       int task_id;
       int priority;      // 0 = highest
       void* callback;    // 関数ポインタ（将来実装）
       void* data;        // タスクデータ
   };
   ```

2. TaskQueue実装（Vectorベース）
   ```cb
   struct TaskQueue<A: Allocator> {
       Vector<Task, A> tasks;
       int next_id;
   };
   
   void task_queue_init<A: Allocator>(TaskQueue<A>& queue);
   void task_queue_push(TaskQueue<A>& queue, Task task);
   Task task_queue_pop(TaskQueue<A>& queue);
   bool task_queue_is_empty(TaskQueue<A>& queue);
   ```

3. 優先度付きキュー（簡易版）
   - Push時にpriorityでソート
   - Pop時は先頭から取得

**Deliverables**:
- `stdlib/async/task.cb`
- `stdlib/async/task_queue.cb`
- Test: `tests/cases/async/test_task_queue.cb`

**Success Criteria**:
- ✅ タスクの追加・取得が動作
- ✅ 優先度順にタスクが処理される
- ✅ 空チェックが正しく動作

---

### Day 2: Event Loop Core (Tuesday)
**Goal**: Event Loopの基本構造実装

**Tasks**:
1. EventLoop構造体
   ```cb
   struct EventLoop<A: Allocator> {
       TaskQueue<A> pending_tasks;
       bool is_running;
       int current_tick;
   };
   ```

2. Event Loop操作
   ```cb
   void event_loop_init<A: Allocator>(EventLoop<A>& loop);
   void event_loop_schedule(EventLoop<A>& loop, Task task);
   void event_loop_run(EventLoop<A>& loop);
   void event_loop_stop(EventLoop<A>& loop);
   ```

3. 基本的な実行ループ
   ```cb
   void event_loop_run(EventLoop<A>& loop) {
       loop.is_running = true;
       
       while (loop.is_running && !task_queue_is_empty(loop.pending_tasks)) {
           Task task = task_queue_pop(loop.pending_tasks);
           println("[EventLoop] Processing task %d", task.task_id);
           // 将来: callback実行
           loop.current_tick++;
       }
       
       println("[EventLoop] Stopped at tick %d", loop.current_tick);
   }
   ```

**Deliverables**:
- `stdlib/async/event_loop.cb`
- Test: `tests/cases/async/test_event_loop.cb`

**Success Criteria**:
- ✅ イベントループの開始・停止
- ✅ タスクが順番に処理される
- ✅ ティックカウントが正しく動作

---

### Day 3: Timer System (Wednesday)
**Goal**: タイマーベースのスケジューリング

**Tasks**:
1. Timer構造体
   ```cb
   struct Timer {
       int timer_id;
       int delay_ms;
       int start_tick;
       void* callback;
       bool is_active;
   };
   ```

2. タイマーキュー
   ```cb
   struct TimerQueue<A: Allocator> {
       Vector<Timer, A> timers;
       int next_id;
   };
   
   void timer_queue_add(TimerQueue<A>& queue, Timer timer);
   void timer_queue_update(TimerQueue<A>& queue, int current_tick);
   ```

3. Event Loopへの統合
   ```cb
   struct EventLoop<A: Allocator> {
       TaskQueue<A> pending_tasks;
       TimerQueue<A> timers;
       bool is_running;
       int current_tick;
   };
   ```

**Deliverables**:
- `stdlib/async/timer.cb`
- Test: `tests/cases/async/test_timer.cb`

**Success Criteria**:
- ✅ タイマーの設定・キャンセル
- ✅ 遅延後のタスク実行
- ✅ 複数タイマーの同時管理

---

### Day 4: sleep_ms Implementation (Thursday)
**Goal**: 基本的な遅延関数の実装

**Tasks**:
1. sleep_ms関数（プレースホルダー版）
   ```cb
   void sleep_ms(int milliseconds) {
       println("[sleep_ms] Sleeping for %d ms", milliseconds);
       // 実際の実装はOS依存
       // 現在はログ出力のみ
   }
   ```

2. Event Loopでのスケジュール遅延
   ```cb
   void event_loop_schedule_delayed(EventLoop<A>& loop, Task task, int delay_ms) {
       Timer timer;
       timer.delay_ms = delay_ms;
       timer.start_tick = loop.current_tick;
       // ... タスクをタイマーキューに追加
   }
   ```

**Deliverables**:
- `stdlib/async/sleep.cb`
- Test: `tests/cases/async/test_sleep.cb`

**Success Criteria**:
- ✅ sleep_ms関数が呼び出せる
- ✅ 遅延スケジュールが動作する
- ✅ タイマーベースの待機が実装される

---

### Day 5: Integration Testing (Friday)
**Goal**: 統合テストと最適化

**Tasks**:
1. 複雑なシナリオテスト
   ```cb
   // 複数タスク + 優先度 + タイマー
   void test_complex_scheduling();
   
   // 異なるアロケータでの動作確認
   void test_different_allocators();
   
   // パフォーマンステスト
   void test_event_loop_performance();
   ```

2. バグ修正とリファクタリング
   - メモリリーク確認
   - エラーハンドリング追加
   - コードクリーンアップ

3. ドキュメント更新
   - API仕様書
   - 使用例
   - パフォーマンス特性

**Deliverables**:
- `tests/cases/async/test_integration.cb`
- `docs/features/event_loop.md`
- Performance benchmark results

**Success Criteria**:
- ✅ すべてのテストが通過
- ✅ メモリリークがない
- ✅ ドキュメントが完成

---

## Technical Challenges

### Challenge 1: Function Pointers
**Issue**: Cbは現在関数ポインタをサポートしていない

**Workaround**:
```cb
struct Task {
    int callback_type;  // 0=TaskA, 1=TaskB, etc.
    void* data;
};

void task_execute(Task& task) {
    if (task.callback_type == 0) {
        task_a_handler(task.data);
    } else if (task.callback_type == 1) {
        task_b_handler(task.data);
    }
}
```

**Future Solution**: Week 4で関数ポインタ実装

### Challenge 2: Type Parameter Method Calls
**Issue**: `A.allocate(size)` が動作しない

**Workaround**: 具体的な型ごとに関数を作成
```cb
void task_queue_init_system(TaskQueue<SystemAllocator>& queue);
void task_queue_init_bump(TaskQueue<BumpAllocator>& queue);
```

**Future Solution**: Week 3後半でパーサー拡張

### Challenge 3: Real-time Precision
**Issue**: インタプリタなので正確なタイミングが難しい

**Workaround**: ティックベースの論理時間
```cb
// 1 tick = 仮想1ミリ秒
int current_tick = 0;
```

**Future Solution**: v0.12.0でコンパイラ実装時に改善

---

## Success Metrics

### Week 3 Completion Criteria
- ✅ TaskQueue実装（push, pop, priority）
- ✅ EventLoop実装（run, stop, schedule）
- ✅ Timer実装（delayed execution）
- ✅ sleep_ms実装（プレースホルダー）
- ✅ 15+ tests passing
- ✅ Documentation complete

### Performance Goals
- TaskQueue操作: O(log n) for priority queue
- EventLoop overhead: < 5% of total execution time
- Memory efficiency: Allocator-dependent, no leaks

### Code Quality
- No memory leaks
- Proper error handling
- Clean API design
- Comprehensive tests

---

## Week 3 Schedule

| Day | Focus | Hours | Deliverables |
|-----|-------|-------|--------------|
| Mon | Task & Queue | 4 | task.cb, task_queue.cb, tests |
| Tue | Event Loop | 4 | event_loop.cb, tests |
| Wed | Timer System | 4 | timer.cb, tests |
| Thu | sleep_ms | 3 | sleep.cb, tests |
| Fri | Integration | 5 | Integration tests, docs |
| **Total** | **Week 3** | **20** | **5 files, 15+ tests** |

---

## Next Steps After Week 3

### Week 4: Future<T> Type
With Event Loop complete, implement:
- Future<T> structure
- State management (Pending, Ready)
- get/set operations
- is_ready checks

### Week 5-6: Result<T, E> + Pattern Matching
- Result<T, E> enum
- match statement
- Error propagation

### Week 7-9: async/await Syntax
- Parser extension for async/await
- Integration with Event Loop
- Full async runtime

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Function pointer limitation | High | Medium | Use callback_type workaround |
| Type parameter issues | Medium | Low | Use concrete types temporarily |
| Schedule delay | Low | Medium | Focus on core features first |
| Memory leaks | Low | High | Comprehensive testing |

---

## Files to Create

```
stdlib/async/
  ├── task.cb              # Task structure
  ├── task_queue.cb        # TaskQueue implementation
  ├── event_loop.cb        # EventLoop core
  ├── timer.cb             # Timer system
  └── sleep.cb             # sleep_ms function

tests/cases/async/
  ├── test_task_queue.cb   # TaskQueue tests
  ├── test_event_loop.cb   # EventLoop tests
  ├── test_timer.cb        # Timer tests
  ├── test_sleep.cb        # sleep_ms tests
  └── test_integration.cb  # Integration tests

docs/features/
  └── event_loop.md        # API documentation
```

---

**Created**: 2025-10-27  
**Owner**: shadowlink0122  
**Status**: Ready to implement  
**Next Review**: 2025-11-03 (Week 3 completion)
