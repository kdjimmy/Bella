
$path = "D:\Bella\build"

Write-Host "检查目录: $path"

# 检查目录是否存在
if (Test-Path $path) {
    try {
        # 尝试强制删除
        Remove-Item -Recurse -Force $path
        Write-Host "目录已成功删除: $path"
    }
    catch {
        Write-Host "删除失败: $($_.Exception.Message)"
        Write-Host "可能原因: 权限不足或目录被占用。"
        Write-Host "建议: 关闭可能占用该目录的程序 (VS/CMake)，然后以管理员身份运行此脚本。"
    }
}
else {
    Write-Host "目录不存在: $path"
}
