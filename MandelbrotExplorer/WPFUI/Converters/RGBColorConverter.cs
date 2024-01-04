using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace WPFUI.Converters
{
    public class RGBColorConverter : IValueConverter
    {
        private Dictionary<uint, string> _nameByRGB = new();
        private Dictionary<string, uint> _rgbByName = new();

        public RGBColorConverter()
        {
            foreach(KnownColor kc in Enum.GetValues<KnownColor>())
            {
                Color color = Color.FromKnownColor(kc);

                if (!color.IsSystemColor)
                {
                    uint rgb = (uint)(color.ToArgb() & 0xFFFFFF);
                    _nameByRGB[rgb] = color.Name;
                    _rgbByName[color.Name] = rgb;
                }
            }
        }

        public uint ConvertBack(string colorName)
        {
            colorName = colorName.Trim();

            if (_rgbByName.ContainsKey(colorName))
                return _rgbByName[colorName];
            else if (uint.TryParse(colorName, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out uint rgb))
                return rgb;
            else
                return 0;
        }

        public string Convert(uint rgb)
        {
            if (_nameByRGB.ContainsKey(rgb))
                return _nameByRGB[rgb];
            else
                return rgb.ToString("x");
        }

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return Convert((uint)value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return ConvertBack(value?.ToString() ?? "");
        }
    }
}
