import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "ShakyLine Documentation Portal",
  description:
    "Download ShakyLine, learn the workflow, and use the control API for deterministic network fault injection.",
  openGraph: {
    title: "ShakyLine Documentation Portal",
    description:
      "A technical download and documentation portal for the ShakyLine network fault injection proxy.",
    url: "https://github.com/sarthakuwar/ShakyLine",
    siteName: "ShakyLine",
    type: "website"
  }
};

export default function RootLayout({
  children
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
