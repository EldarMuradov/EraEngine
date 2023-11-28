namespace EraScriptingCore.FileSystem;

public class FileCloner : IFileCloner
{
    public void Clone(string input, string output)
    {
        DirectoryInfo inSource = new DirectoryInfo(input);
        DirectoryInfo outTarget = new DirectoryInfo(output);

        CopyAll(inSource, outTarget);
    }

    private static void CopyAll(DirectoryInfo source, DirectoryInfo target)
    {
        Directory.CreateDirectory(target.FullName);

        foreach (FileInfo fi in source.GetFiles())
        {
            fi.CopyTo(Path.Combine(target.FullName, fi.Name), true);
        }

        foreach (DirectoryInfo diSourceSubDir in source.GetDirectories())
        {
            DirectoryInfo nextTargetSubDir =
                target.CreateSubdirectory(diSourceSubDir.Name);
            CopyAll(diSourceSubDir, nextTargetSubDir);
        }
    }
}
