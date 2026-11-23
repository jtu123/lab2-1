use std::error::Error;
use std::io::{self, Write};

// 같은 입력 과정을 A와 B에서 재사용한다.
fn read_matrix(name: &str) -> Result<Vec<Vec<i64>>, Box<dyn Error>> {
    print!("{} rows columns: ", name);
    io::stdout().flush()?;
    let mut line = String::new();
    io::stdin().read_line(&mut line)?;
    let size: Vec<usize> = line.split_whitespace()
        .map(str::parse).collect::<Result<_, _>>()?;
    if size.len() != 2 || size[0] == 0 || size[1] == 0 {
        return Err("행과 열은 양의 정수 두 개여야 합니다.".into());
    }

    let (rows, cols) = (size[0], size[1]);
    let mut matrix = Vec::with_capacity(rows);
    for r in 0..rows {
        print!("{} row {} ({} integers): ", name, r + 1, cols);
        io::stdout().flush()?;
        line.clear();
        io::stdin().read_line(&mut line)?;
        let row: Vec<i64> = line.split_whitespace()
            .map(str::parse).collect::<Result<_, _>>()?;
        if row.len() != cols {
            return Err("한 행에 입력한 원소 수가 열 수와 다릅니다.".into());
        }
        matrix.push(row);
    }
    Ok(matrix)
}

fn main() -> Result<(), Box<dyn Error>> {
    let a = read_matrix("A")?;
    let b = read_matrix("B")?;
    if a.len() != b.len() || a[0].len() != b[0].len() {
        return Err("두 행렬의 행 수와 열 수가 같아야 합니다.".into());
    }

    let (rows, cols) = (a.len(), a[0].len());
    let mut sum = vec![vec![0i64; cols]; rows];
    for r in 0..rows {
        for c in 0..cols {
            sum[r][c] = a[r][c].checked_add(b[r][c])
                .ok_or("행렬 덧셈에서 i64 범위를 벗어났습니다.")?;
        }
    }

    println!("A + B:");
    for row in &sum {
        for value in row {
            print!("{} ", value);
        }
        println!();
    }
    Ok(())
}
