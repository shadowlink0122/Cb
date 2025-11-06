# Queue&lt;T&gt; - ジェネリックFIFOキュー

## 概要

`Queue<T>`は先入れ先出し（FIFO）のデータ構造です。双方向リンクリストで実装されており、効率的なenqueue/dequeue操作を提供します。

## インポート

```cb
import stdlib.std.queue;
```

## 基本的な使い方

```cb
Queue<int> q;
q.push(10);      // enqueue
q.push(20);
q.push(30);

int first = q.pop();  // dequeue - returns 10
int top = q.top();    // peek - returns 20 (without removing)
int size = q.size();  // returns 2
bool empty = q.is_empty();  // returns false
```

## メソッド

| メソッド | 説明 | 時間計算量 |
|---------|------|-----------|
| `push(T value)` | 要素を末尾に追加 | O(1) |
| `pop()` | 先頭要素を削除して返す | O(1) |
| `top()` | 先頭要素を返す（削除しない） | O(1) |
| `size()` | 要素数を返す | O(1) |
| `is_empty()` | 空かどうか | O(1) |
| `clear()` | 全要素を削除 | O(n) |

## サポートされる型

- プリミティブ型: `int`, `long`, `float`, `double`, `string`, `bool`
- 構造体
- ネストしたジェネリクス: `Queue<Vector<int>>`

## 使用例

### 基本例

```cb
import stdlib.std.queue;

void main() {
    Queue<string> tasks;
    tasks.push("Task 1");
    tasks.push("Task 2");
    tasks.push("Task 3");
    
    while (!tasks.is_empty()) {
        string task = tasks.pop();
        println("Processing: {task}");
    }
}
```

### 構造体とQueue

```cb
import stdlib.std.queue;

struct Job {
    int id;
    string description;
};

void main() {
    Queue<Job> job_queue;
    
    Job j1;
    j1.id = 1;
    j1.description = "First job";
    job_queue.push(j1);
    
    Job next_job = job_queue.pop();
    println("Job {next_job.id}: {next_job.description}");
}
```

## パフォーマンス

- **時間計算量**: 全ての主要操作がO(1)
- **空間計算量**: O(n) - 各要素に2つのポインタ

## 関連項目

- [Vector](./vector.md)
- [Map](./map.md)
