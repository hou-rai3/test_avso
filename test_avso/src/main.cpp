#include <mbed.h>   // Mbed OSの基本機能を提供するヘッダファイル
#include "Rs485.h"  // RS-485通信を扱うためのカスタムライブラリ

// IOピンとシリアル通信設定
BufferedSerial pc{USBTX, USBRX, 115200};  // USB経由でPCと通信するためのシリアルポート（115200bps）
Rs485 rs485{PB_6, PA_10, (int)2e6, PC_0}; // RS-485通信のインスタンス、送信ピン、受信ピン、通信速度、制御ピンの設定
Timer timer;                              // 時間計測用のタイマー

// AMT21エンコーダーのデータを処理するための構造体
struct Amt21 {
  static constexpr int rotate = 4096;  // エンコーダーの回転数の分解能

  uint8_t address;   // エンコーダーのアドレス
  int32_t pos;       // エンコーダーの現在位置
  uint16_t pre_pos;  // 前回の位置

  // 現在位置をエンコーダーから要求し、位置を更新する関数
  bool request_pos() {
    rs485.uart_transmit({address});  // エンコーダーに現在位置を要求
    if(uint16_t now_pos; rs485.uart_receive(&now_pos, sizeof(now_pos), 10ms) && is_valid(now_pos)) {
      now_pos = (now_pos & 0x3fff) >> 2;  // データのフォーマットを調整して位置を取得
      int16_t diff = now_pos - pre_pos;   // 前回の位置との差分を計算
      if(diff > rotate / 2) {             // 差分が大きすぎる場合の補正
        diff -= rotate;
      } else if(diff < -rotate / 2) {
        diff += rotate;
      }
      pos += diff;        // 現在の位置を更新
      pre_pos = now_pos;  // 前回の位置を更新
      return true;        // 正常に受信できたことを返す
    }
    return false;         // 受信に失敗した場合
  }

  // エンコーダーをリセットするための関数
  void request_reset() {
    rs485.uart_transmit({address + 2, 0x75});  // リセット命令を送信
  }

  // 受信したデータが有効かどうかを確認する関数
  static bool is_valid(uint16_t raw_data) {
    bool k1 = raw_data >> 15;               // パリティビットのチェック（偶数）
    bool k0 = raw_data >> 14 & 1;           // パリティビットのチェック（奇数）
    raw_data <<= 2;
    do {
      k1 ^= raw_data & 0x8000;              // 偶数パリティチェックを進める
      k0 ^= (raw_data <<= 1) & 0x8000;      // 奇数パリティチェックを進める
    } while(raw_data <<= 1);
    return k0 && k1;  // パリティチェックの結果が正しければtrueを返す
  }
} amt[] = {{0x50}, {0x54}, {0x58}, {0x5C}};  // 4つのAMT21エンコーダーを設定（異なるアドレス）

/// @brief プログラムのエントリーポイント
int main() {
  printf("\nsetup\n");    // プログラム開始時に"setup"と表示
  timer.start();          // タイマーを開始
  auto pre = timer.elapsed_time();  // 開始時間を記録

  while(1) {
    auto now = timer.elapsed_time();  // 現在の時間を取得
    if(now - pre > 20ms) {            // 20ミリ秒ごとに処理を行う
      printf("hoge\n");               // "hoge"を表示（デバッグ用）

      for(auto& e: amt) {             // 各エンコーダーについて
        e.request_pos();              // 現在位置を要求
        printf("% 12ld ", e.pos);     // 現在の位置を表示
      }

      pre = now;                      // 次回の処理のために時間を更新
    }
  }
}

// ここには追加の関数定義が入ることが想定される