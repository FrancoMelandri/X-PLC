
using System;

namespace XPLC {
	class XPLC {
		static void Main(string[] arg) {
			if ( arg.Length == 0 )
			{
					Console.WriteLine( "USAGE: XPLC.Compiler.exe <filename>");
			}
			else
			{
				Scanner scanner = new Scanner(arg[0]);
				Parser parser = new Parser(scanner);
				parser.Parse();
				if ( parser.errors.count > 0 )
					Console.WriteLine(parser.errors.count + " errors detected");
				else
					Console.WriteLine( arg[0] + " successfully compiled.");
			}
		}
	}
}