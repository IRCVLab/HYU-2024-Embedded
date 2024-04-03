# Week 05. 프로세스 스케줄러

## 1. 스케줄러 확인 및 변경 예제

- 현재 기본 값으로 설정된 스케줄링 알고리즘이 어떤 것인지 확인
- 다른 스케줄링 알고리즘으로 변경
- 소스코드: `src/sched_alg.c`

### 1-1. 컴파일
- 컴파일하여 `output` 폴더 아래에 실행 파일 생성

  ```sh
  $ gcc -o output/sched_alg src/sched_alg.c
  ```

### 1-2. 실행

- 프로그램 실행
  ```sh
  $ sudo output/sched_alg
  ```

> [!IMPORTANT]
> `sched_alg.c`는 root 권한이 필요한 작업(스케줄링 알고리즘 변경)을 포함하기 때문에 root 권한으로 실행해야 함.



## 2. 멀티 프로세스 실행 순서 확인

- CPU가 특정 시점에 어떤 프로세스를 실행 중인지 확인

- 테스트 프로그램: `src/sched.c`
  - 명령어 라인 파라미터(nproc, total, resolution, policy)

    |파라미터|설명|
    |:-:|:-:|
    |nproc|동시 동작 프로세스 수|
    |total|프로그램이 동작하는 총 시간\[ms\]|
    |resolution|데이터 수집 시간 간격\[ms\]|
    |policy|스케줄링 방법|

  - 각 프로세스의 동작 방식
    - CPU 시간을 total 밀리초만큼 사용한 후 종료
    - CPU 시간을 resolution 밀리초만큼 사용할 때마다 다음 내용을 출력

      ```sh
      프로세스고유ID(0 ~ nproc-1)   경과한시간   진행도[%]
      ```

### 2-1. 테스트 프로그램 컴파일

```sh
$ gcc -o output/sched src/sched.c
```

### 2-2. 테스트 프로그램 실행 방법

- 프로그램 실행
  - 1개의 프로세스 사용 (nproc = 1)
  - 100ms 수행 (total = 100)
  - 데이터 수집 간격 1ms (resolution = 1)
  - 스케줄링 알고리즘 (policy = SCHED_RR)

    ```sh
        $ sudo ./output/sched 1 100 1 SCHED_RR
    ```

- 논리 CPU 갯수 지정: `taskset`
  - 모든 프로세스가 **특정 갯수의 논리 CPU에서만 동작**하게 하여 스케줄러의 동작을 확인
  - 아래 형식처럼 **taskset** 명령어의 **-c** 옵션으로 논리 CPU를 지정

    ```sh
    $ sudo taskset -c 0 ./output/sched nproc total resolution policy
    ```

- 프로세스의 개수를 3가지 경우(1개, 2개, 4개)로 나눠서 결과를 측정

### 2-3. 실험 결과를 파일로 저장

- 논리 CPU 1개에서 프로세스 1개를 100ms동안 1ms 정밀도로 측정하며 동작

  ```sh
  $ sudo taskset -c 0 output/sched 1 100 1 SCHED_RR
  0       1       1
  0       1       2
  0       2       3
  ...
  0       92      99
  0       93      100
  ```

- 결과를 파일로 저장하기 위해서 리다이렉션(Redirection) 사용

  ```sh
  $ sudo taskset -c 0 output/sched 1 100 1 SCHED_RR > log/1core-1process.log
  $ sudo taskset -c 0 output/sched 2 100 1 SCHED_RR > log/1core-2process.log
  $ sudo taskset -c 0 output/sched 4 100 1 SCHED_RR > log/1core-4process.log
  ```

### 2-4. 프로세스 개수에 따른 진행도 시각화

- [gnuplot](gnuplot.md) 툴을 이용하여 각 결과를 시각화하여 확인

  ```sh
  $ gnuplot -e "data='log/1core-1process.log';filename='result/1core-1process.png'" plot/1process.gnu
  $ gnuplot -e "data='log/1core-2process.log';filename='result/1core-2process.png'" plot/2process.gnu
  $ gnuplot -e "data='log/1core-4process.log';filename='result/1core-4process.png'" plot/4process.gnu
  ```

### 2-5. 논리 CPU의 타임 슬라이스 시각화
  ```sh
  $ sudo taskset -c 0,1,2,3 output/sched 1 100 1 SCHED_RR > log/4core-1process.log
  $ sudo taskset -c 0,1,2,3 output/sched 2 100 1 SCHED_RR > log/4core-2process.log
  $ sudo taskset -c 0,1,2,3 output/sched 4 100 1 SCHED_RR > log/4core-4process.log

  $ gnuplot -e "data='log/4core-1process.log';filename='result/4core-1process.png'" plot/1process.gnu
  $ gnuplot -e "data='log/4core-2process.log';filename='result/4core-2process.png'" plot/2process.gnu
  $ gnuplot -e "data='log/4core-4process.log';filename='result/4core-4process.png'" plot/4process.gnu
  ```



## 3. 프로세스 스케줄링 변경

### 3-1. FIFO

- 테스트 프로그램 실행 & 시각화
  ```sh
  $ sudo taskset -c 0 ./output/sched 4 100 1 SCHED_FIFO > log/1core-4process.log
  $ gnuplot -e "data='log/1core-4process.log';filename='result/1core-4process_FIFO.png'" plot/4process.gnu
  ```


### 3-2. Round Robin

- 테스트 프로그램 실행 & 시각화
  ```sh
  $ sudo taskset -c 0 ./output/sched 4 100 1 SCHED_RR > log/1core-4process.log
  $ gnuplot -e "data='log/1core-4process.log';filename='result/1core-4process_RR.png'" plot/4process.gnu
  ```


### 3-3. Round Robin (delta = 10ms)

- 기본 Timeslice 확인
  ```sh
  $ cat /proc/sys/kernel/sched_rr_timeslice_ms
  ```

- Timeslice 변경
  ```sh
  $ echo "20" | sudo tee /proc/sys/kernel/sched_rr_timeslice_ms
  ```

- 테스트 프로그램 실행 & 시각화
  ```sh
  $ sudo taskset -c 0 ./output/sched 4 100 1 SCHED_RR > log/1core-4process.log
  $ gnuplot -e "data='log/1core-4process.log';filename='result/1core-4process_RR_20ms.png'" plot/4process.gnu
  ```


