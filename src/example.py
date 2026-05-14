import time
import csv  # 💡 CSV 저장을 위한 기본 모듈 추가
from pybela import Streamer

# python3 example.py

# =====================================================================
# 💡 [CSV 파일 준비]
# w 모드로 파일을 열어 프로그램이 실행될 때마다 새 파일로 덮어씁니다.
# 누적해서 저장하려면 'w' 대신 'a'를 사용하세요.
# =====================================================================
csv_filename = "trill_data_log.csv"
csv_file = open(csv_filename, mode='w', newline='')
csv_writer = csv.writer(csv_file)

# CSV 첫 줄에 들어갈 헤더(목차) 작성
csv_writer.writerow(["Time_Seconds", "Variable_Name", "Data_Values..."])


# 1. AI 두뇌 역할 (콜백 함수)
def process_touch_data(msg):
    var_name = msg["name"]
    sensor_values = msg["buffer"]["data"]
    
    # 1. 터치 없는 데이터 필터링
    if sum(sensor_values) == 0.0:
        return
        
    # =====================================================================
    # 💡 [핵심 해결책: 데이터 다운샘플링]
    # 리스트 슬라이싱 [::16]을 사용하면 처음부터 끝까지 '16칸 간격'으로 값을 가져옵니다.
    # 즉, 0번(1열), 16번(17열), 32번(33열) 인덱스의 값만 뽑아내어 새로운 리스트를 만듭니다.
    # (1024개였던 데이터가 64개로 줄어듭니다!)
    # =====================================================================
    unique_values = sensor_values[::16]
    
    # 2. [시간 정보 추출] 
    ref_frame = msg["buffer"]["ref_timestamp"]
    time_seconds = ref_frame / 44100.0
    
    # 터미널 출력 (몇 개로 줄었는지 확인하기 위해 len(unique_values) 출력 추가)
    print(f"[{var_name}] 발생 시간: {time_seconds:.3f}초 | 데이터({len(unique_values)}개): {unique_values[:5]}...")

    # -------------------------------------------------------------------
    # 💡 [CSV 기록 로직]
    # 기존의 중복 많은 sensor_values 대신, 간격별로 뽑아낸 unique_values를 저장합니다.
    # -------------------------------------------------------------------
    row_data = [f"{time_seconds:.5f}", var_name] + unique_values
    csv_writer.writerow(row_data)
    
    # 파일에 즉시 쓰기 강제 
    csv_file.flush()


def main():
    streamer = Streamer(ip="192.168.7.2") 
    
    print("Bela 보드에 연결을 시도합니다...")
    try:
        streamer.connect()
        print("✅ Bela 보드와 연결 성공!")
    except Exception as e:
        print(f"❌ 연결 실패: {e}")
        return

    target_variables = ["touch_location", "touch_size"]

    print(f"📡 {target_variables} 데이터 스트리밍을 시작합니다. (종료하려면 Ctrl+C)")
    print(f"💾 데이터는 '{csv_filename}' 파일에 실시간으로 저장됩니다.")
    
    streamer.start_streaming(
        variables=target_variables,
        on_buffer_callback=process_touch_data
    )

    try:
        streamer.wait(3600)
    except KeyboardInterrupt:
        print("\n사용자에 의해 프로그램이 중단되었습니다.")
    finally:
        # 종료 시 안전하게 스트리밍 중지 및 연결 해제
        streamer.stop_streaming()
        streamer.disconnect()
        
        # 💡 [CSV 파일 닫기] 사용이 끝난 파일 객체를 안전하게 닫아줍니다.
        csv_file.close()
        print(f"🛑 스트리밍이 종료되었으며, 데이터가 {csv_filename}에 안전하게 저장되었습니다.")

if __name__ == "__main__":
    main()
